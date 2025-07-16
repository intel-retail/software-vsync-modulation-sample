/*
 * Copyright © 2024 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

#include <math.h>
#include <debug.h>
#include <signal.h>
#include <unistd.h>
#include <memory.h>
#include <time.h>
#include "mmio.h"
#include "combo.h"

combo_phy_reg combo_table[] = {
	{REG(DPLL0_CFGCR0), REG(DPLL0_CFGCR1), 0},
	{REG(DPLL1_CFGCR0), REG(DPLL1_CFGCR1), 0},
	{REG(DPLL2_CFGCR0), REG(DPLL2_CFGCR1), 0},
	{REG(DPLL3_CFGCR0), REG(DPLL3_CFGCR1), 0},
};

div_val pdiv_table[4] = {
	{1, 2},
	{2, 3},
	{4, 5},
	{8, 7},
};

div_val kdiv_table[3] = {
	{1, 1},
	{2, 2},
	{4, 3},
};

register_info reg_cfgcr0 = { "CFGCR0",
							{{"DCO Integer", 0, 9},  // 10 bits
							{"DCO Fraction", 10, 24}, // 15 bits
							{"SSC enable", 25, 25},
							{"", 0, 0}} };

register_info reg_cfgcr1 = { "CFGCR1",
							{{"C Frequency", 0, 1},
							{"Pdiv", 2, 5},
							{"Kdiv", 6, 8},
							{"Qdiv Mode", 9, 9},
							{"Qdiv Ratio", 10, 17},
							{"", 0, 0}} };

/**
 * @brief Constructor for a new combo::combo
 *
 * @param ds
 * @param pipe
 */
combo::combo(ddi_sel* ds, int _pipe) : phys(_pipe)
{
	TRACING();
	int dpclk;
	combo_phy = {};
	dpll_num = 0;

	if (!ds) {
		ERR("ddi_sel pointer is null\n");
		return;
	}

	// Read DPCLKA_CFGCR0 or DPCLKA_CFGCR1 depending upon which DDI is enabled
	dpclk = READ_OFFSET_DWORD(ds->dpclk.addr);
	DBG("0x%X = 0x%X\n", ds->dpclk.addr, dpclk);
	// DPCLKA_CFGCR must have the DDI's clock bit set to 0 (turned on)
	if (dpclk & BIT(ds->clock_bit)) {
		ERR("Clock is Off. dpclk = 0x%X, clock bit = 0x%X\n", dpclk, ds->clock_bit);
		return;
	}

	// Since there are only 2 bits for all DPLLs, it is safe to
	// assume that the high order bit is just one more than the
	// low order bit
	ds->dpll_num = GETBITS_VAL(dpclk,
		ds->mux_select_low_bit + 1,
		ds->mux_select_low_bit);
	DBG("DPLL num = 0x%X\n", ds->dpll_num);

	if (ds->dpll_num >= ARRAY_SIZE(combo_table)) {
		ERR("Dpll number (0x%X) is higher than combo table size.", ds->dpll_num);
		return;
	}

	// One PHY should be connected to one display only
	if (!combo_table[ds->dpll_num].enabled) {
		combo_table[ds->dpll_num].enabled = 1;
		combo_phy = &combo_table[ds->dpll_num];
		set_ds(ds);
		set_init(true);
		done = 1;
	}

	phy_type = COMBO;
}

/**
 * @brief Destructor for combo
 *
 */
combo::~combo()
{
	ddi_sel* ds = get_ds();
	combo_table[ds->dpll_num].enabled = 0;
}

/**
 * @brief
 * This function called by phy class and delegate calculation
 * to overloaded function.
 *
 * @return double - The calculated PLL clock
 */
double combo::calculate_pll_clock()
{
	TRACING();
	uint32_t cfgcr0 = READ_OFFSET_DWORD(combo_phy->cfgcr0.addr);
	uint32_t cfgcr1 = READ_OFFSET_DWORD(combo_phy->cfgcr1.addr);
	return calculate_pll_clock(cfgcr0, cfgcr1);
}

/**
 * @brief
 * This function calculates pll clock base on the cfgcr0 and cfgcr1 values
 *
 * @param cfgcr0 - The cfgcr0 register value
 * @param cfgcr1 - The cfgcr1 register value
 * @return double - The calculated PLL clock
 */
double combo::calculate_pll_clock(uint32_t cfgcr0, uint32_t cfgcr1)
{
	TRACING();
	double refclock = REF_COMBO_FREQ * 1000;
	int i_fbdiv_intgr_9_0 = GETBITS_VAL(cfgcr0, 9, 0);
	int i_fbdivfrac_14_0 = GETBITS_VAL(cfgcr0, 24, 10);
	double dco_freq = i_fbdiv_intgr_9_0 * refclock + ((i_fbdivfrac_14_0 * 2 * refclock) / 0x8000);
	return dco_freq / 1000.0;  // Return as Khz
}

/**
 * @brief
 * This function calculates the feedback dividers for the PLL frequency
 *
 * @param pll_clock - The desired PLL frequency
 * @return 0 - success, non zero - failure
 */
int combo::calculate_feedback_dividers(double pll_clock)
{
	TRACING();
	// Construct integer and fraction parts
	uint32_t new_i_fbdiv_intgr_9_0 = 0;
	uint32_t new_i_fbdivfrac_14_0 = 0;

	uint32_t cfgcr0 = READ_OFFSET_DWORD(combo_phy->cfgcr0.addr);
	uint32_t cfgcr1 = READ_OFFSET_DWORD(combo_phy->cfgcr1.addr);
	uint32_t cfgcr0_mod_val = cfgcr0;

	calculate_feedback_dividers(pll_clock, cfgcr1, &new_i_fbdiv_intgr_9_0, &new_i_fbdivfrac_14_0);

	cfgcr0_mod_val &= ~GENMASK(24, 0);
	cfgcr0_mod_val |= new_i_fbdiv_intgr_9_0;
	cfgcr0_mod_val |= (long)new_i_fbdivfrac_14_0 << 10;

	uint32_t old_i_fbdiv_intgr_9_0 = GETBITS_VAL(cfgcr0, 9, 0);
	uint32_t old_i_fbdivfrac_14_0 = GETBITS_VAL(cfgcr0, 24, 10);
	INFO("\ti_fbdiv_intgr_9_0: 0x%X [%d] -> 0x%X [%d]\n", old_i_fbdiv_intgr_9_0, old_i_fbdiv_intgr_9_0, (int)new_i_fbdiv_intgr_9_0, (int)new_i_fbdiv_intgr_9_0);
	INFO("\ti_fbdivfrac_14_0: 0x%X [%d] -> 0x%X [%d]\n", old_i_fbdivfrac_14_0, old_i_fbdivfrac_14_0, (int)new_i_fbdivfrac_14_0, (int)new_i_fbdivfrac_14_0);
	INFO("\tcfgcr0[0x%X]: 0x%X -> 0x%X\n", combo_phy->cfgcr0.addr, cfgcr0, cfgcr0_mod_val);
	INFO("\tcfgcr1[0x%X]: 0x%X\n", combo_phy->cfgcr1.addr, cfgcr1);

	combo_phy->cfgcr0.mod_val = cfgcr0_mod_val;
	// Update cfgcr1 even if it's not modified
	combo_phy->cfgcr1.mod_val = cfgcr1;

	return 0;
}

/**
 * @brief
 * This function calculates the feedback dividers for the PLL frequency
 *
 * @param pll_freq - The desired PLL frequency
 * @param cfgcr1 - The cfgcr1 register value.  Not used for the time being.
 * @param i_fbdiv_intgr_9_0 - Return value: The integer part of the feedback divider
 * @param i_fbdivfrac_14_0 - Return value: The fractional part of the feedback divider
 */
void combo::calculate_feedback_dividers(double pll_clock, uint32_t cfgcr1, uint32_t* i_fbdiv_intgr_9_0, uint32_t* i_fbdivfrac_14_0)
{
	TRACING();
	if (!i_fbdiv_intgr_9_0 || !i_fbdivfrac_14_0) {
		ERR("Error: Null pointer for output values.\n");
		return;
	}

	double refclock = REF_COMBO_FREQ * 1000;
	pll_clock *= 1000;  // Convert to KHz
	// Calculate the integer part
	*i_fbdiv_intgr_9_0 = (uint32_t)(pll_clock / refclock);

	// Calculate the remainder
	uint64_t remainder = pll_clock - ((uint64_t)(*i_fbdiv_intgr_9_0) * refclock);

	// Calculate the fractional part
	*i_fbdivfrac_14_0 = (remainder * 0x8000) / (2 * refclock);
}

/**
 * @brief
 * This function reads the Combo Phy MMIO registers
 * @param None
 * @return void
 */
void combo::read_registers()
{
	TRACING();
	combo_phy->cfgcr0.orig_val = READ_OFFSET_DWORD(combo_phy->cfgcr0.addr);
	combo_phy->cfgcr1.orig_val = READ_OFFSET_DWORD(combo_phy->cfgcr1.addr);
}

/**
 * @brief
 * This function prints the register value
 *
 * @param None
 * @return void
 */
void combo::print_registers()
{
	TRACING();
	PRINT("Original Value ->\n");
	print_register(combo_phy->cfgcr0.orig_val, combo_phy->cfgcr0.addr, &reg_cfgcr0);
	print_register(combo_phy->cfgcr1.orig_val, combo_phy->cfgcr1.addr, &reg_cfgcr1);

	PRINT("Updated Value ->\n");
	print_register(combo_phy->cfgcr0.mod_val, combo_phy->cfgcr0.addr, &reg_cfgcr0);
	print_register(combo_phy->cfgcr1.mod_val, combo_phy->cfgcr1.addr, &reg_cfgcr1);
}

/**
* @brief
* This function programs the Combo Phy MMIO registers
* needed to move avsync period for a system.
* @param mod - This parameter tells the function whether to program the original
* values or the modified ones.
* - 0 = Original
* - 1 = modified
* @return 0 - success, non zero - failure
*/
int combo::program_mmio(int mod)
{
	TRACING();
	WRITE_OFFSET_DWORD(combo_phy->cfgcr0.addr,
		mod ? combo_phy->cfgcr0.mod_val : combo_phy->cfgcr0.orig_val);

	if (!mod) {
		DBG("DEFAULT VALUES\n cfgcr0 [0x%X] =\t 0x%X\n cfgcr1 [0x%X] =\t 0x%X\n",
			combo_phy->cfgcr0.addr, combo_phy->cfgcr0.orig_val, combo_phy->cfgcr1.addr, combo_phy->cfgcr1.orig_val);
	}

	return 0;
}

