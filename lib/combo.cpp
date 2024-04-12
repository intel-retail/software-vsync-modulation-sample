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
	{REG(DPLL0_CFGCR0), REG(DPLL0_CFGCR1), 0, 1},
	{REG(DPLL1_CFGCR0), REG(DPLL1_CFGCR1), 0, 1},
	{REG(DPLL2_CFGCR0), REG(DPLL2_CFGCR1), 0, 1},
	{REG(DPLL3_CFGCR0), REG(DPLL3_CFGCR1), 0, 1},
};

combo::combo(ddi_sel *ds)
{
	int dpclk;

	// Read DPCLKA_CFGCR0 or DPCLKA_CFGCR1 depending upon which DDI is enabled
	dpclk = READ_OFFSET_DWORD(ds->dpclk.addr);
	DBG("0x%X = 0x%X\n", ds->dpclk.addr, dpclk);
	// DPCLKA_CFGCR must have the DDI's clock bit set to 0 (turned on)
	if(dpclk & BIT(ds->clock_bit)) {
		ERR("Clock is Off. dpclk = 0x%X, clock bit = 0x%X\n", dpclk, ds->clock_bit);
		return;
	}

	// Since there are only 2 bits for all DPLLs, it is safe to
	// assume that the high order bit is just one more than the
	// low order bit
	ds->dpll_num = GETBITS_VAL(dpclk,
			ds->mux_select_low_bit+1,
			ds->mux_select_low_bit);
	DBG("DPLL num = 0x%X\n", ds->dpll_num);

	if(ds->dpll_num >= ARRAY_SIZE(combo_table)) {
		ERR("Dpll number (0x%X) is higher than combo table size.", ds->dpll_num);
		return;
	}
	// One PHY should be connected to one display only
	if(!combo_table[ds->dpll_num].enabled) {
		combo_table[ds->dpll_num].enabled = 1;
		ds->phy_data = &combo_table[ds->dpll_num];
		set_ds(ds);
		set_init(true);
	}
}

/**
* @brief
* This function finds the value from a bit within a table
* @param *dt - This is the table to search for a val from.
* @param dt_size - The size of this table
* @param bit - The bit to search for
* @return The value to return corresponding to the bit
*/
int get_val_from_bit(div_val *dt, int dt_size, int bit)
{
	int i, val = 0;
	for(i = 0; i < dt_size; i++) {
		if(dt[i].bit == bit) {
			val = dt[i].val;
			break;
		}
	}
	if(i >= dt_size) {
		ERR("Couldn't find the right val from the table given this bit\n");
	}
	return val;
}

/**
* @brief
* This function programs Combo phys on the system
* @param time_diff - This is the time difference in between the primary and the
* secondary systems in ms. If master is ahead of the slave , then the time
* difference is a positive number otherwise negative.
* @param *t - A pointer to a pointer where we need to store the timer
* @return void
*/
void combo::program_phy(double time_diff)
{
	double shift = SHIFT;
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
	ddi_sel *ds = get_ds();
	combo_phy_reg *combo_phy = (combo_phy_reg *) ds->phy_data;

#if TESTING
	// ADL
	/*
	combo_phy->cfgcr0.orig_val = 0x01c001a5;
	combo_phy->cfgcr1.orig_val = 0x013331cf;
	*/
	// TGL
	combo_phy->cfgcr0.orig_val = 0x00B001B1;
	combo_phy->cfgcr1.orig_val = 0x00000e84;
#else
	READ_VAL(cfgcr0, orig_val);
	READ_VAL(cfgcr1, orig_val);

	if(time_diff < 0) {
		shift *= -1;
	}

	// For whichever PHY we find, let's set the done flag to 0 so that we can later
	// have a timer for it to reset the default values back in their registers
	combo_phy->done = 0;
	int steps = calc_steps_to_sync(time_diff, shift);
	DBG("steps are %d\n", steps);
	user_info *ui = new user_info(this, combo_phy);
	make_timer((long) steps, ui, reset_phy_regs);
#endif
	DBG("OLD VALUES\n cfgcr0 [0x%X] =\t 0x%X\n cfgcr1 [0x%X] =\t 0x%X\n",
			combo_phy->cfgcr0.addr, combo_phy->cfgcr0.orig_val, 
			combo_phy->cfgcr1.addr, combo_phy->cfgcr1.orig_val);

	// Symbol clock frequency in MHz (base) = DCO Divider// Reference frequency in MHz /  (5// Pdiv// Qdiv// Kdiv)
	// DCO Divider comes from DPLL_CFGCR0 DCO Integer + (DPLL_CFGCR0 DCO Fraction / 2^15)
	// Pdiv from DPLL_CFGCR1 Pdiv
	// Qdiv from DPLL_CFGCR1 Qdiv Mode ? DPLL_CFGCR1 Qdiv Ratio : 1
	// Kdiv from DPLL_CFGCR1 Kdiv
	int i_fbdiv_intgr_9_0 = GETBITS_VAL(combo_phy->cfgcr0.orig_val, 9, 0);
	int i_fbdivfrac_14_0 = GETBITS_VAL(combo_phy->cfgcr0.orig_val, 24, 10);
	int pdiv_bit = GETBITS_VAL(combo_phy->cfgcr1.orig_val, 5, 2);
	int pdiv = get_val_from_bit(pdiv_table, ARRAY_SIZE(pdiv_table), pdiv_bit);
	int kdiv_bit = GETBITS_VAL(combo_phy->cfgcr1.orig_val, 8, 6);
	int kdiv = get_val_from_bit(kdiv_table, ARRAY_SIZE(kdiv_table), kdiv_bit);
	int qdiv_mode = combo_phy->cfgcr1.orig_val & BIT(9);
	/* TODO: In case we run into some other weird dividers, then we may need to revisit this */
	int qdiv = (kdiv == 2 && qdiv_mode == 1) ? GETBITS_VAL(combo_phy->cfgcr1.orig_val, 17, 10) : 1;
	int ro_div_bias_frac = i_fbdivfrac_14_0 << 5 | ((i_fbdiv_intgr_9_0 & GENMASK(2, 0)) << 19);
	int ro_div_bias_int = i_fbdiv_intgr_9_0 >> 3;
	double dco_divider = 4*((double) ro_div_bias_int + ((double) ro_div_bias_frac / pow(2,22)));
	double dco_clock = 2 * REF_COMBO_FREQ * dco_divider;
	double pll_freq = dco_clock / (5 * pdiv * qdiv * kdiv);
	double new_pll_freq = pll_freq + (shift * pll_freq / 100);
	double new_dco_clock = dco_clock + (shift * dco_clock / 100);
	double new_dco_divider = new_dco_clock / (2 * REF_COMBO_FREQ);
	double new_ro_div_bias_frac = (new_dco_divider / 4 - (double) ro_div_bias_int) * pow(2,22);
	if(new_ro_div_bias_frac > 0xFFFFF) {
		i_fbdiv_intgr_9_0 += 1;
		new_ro_div_bias_frac -= 0xFFFFF;
	} else if (new_ro_div_bias_frac < 0) {
		i_fbdiv_intgr_9_0 -= 1;
		new_ro_div_bias_frac += 0xFFFFF;
	}
	double new_i_fbdivfrac_14_0  = ((long int) new_ro_div_bias_frac & ~BIT(19)) >> 5;

	DBG("old pll_freq \t %f\n", pll_freq);
	DBG("new_pll_freq \t %f\n", new_pll_freq);
	DBG("old dco_clock \t %f\n", dco_clock);
	DBG("new dco_clock \t %f\n", new_dco_clock);
	DBG("old fbdivfrac \t 0x%X\n", (int) i_fbdivfrac_14_0);
	DBG("old ro_div_frac \t 0x%X\n", ro_div_bias_frac);
	DBG("old fbdivint \t 0x%X\n", i_fbdiv_intgr_9_0);
	DBG("old ro_div_int \t 0x%X\n", ro_div_bias_int);
	DBG("new fbdivfrac \t 0x%X\n", (int) new_i_fbdivfrac_14_0);
	DBG("new ro_div_frac \t 0x%X\n", (int) new_ro_div_bias_frac);

	combo_phy->cfgcr0.mod_val &= ~GENMASK(24, 0);
	combo_phy->cfgcr0.mod_val |= i_fbdiv_intgr_9_0;
	combo_phy->cfgcr0.mod_val |= (long) new_i_fbdivfrac_14_0 << 10;

	DBG("NEW VALUES\n cfgcr0 [0x%X] =\t 0x%X\n", combo_phy->cfgcr0.addr,
	combo_phy->cfgcr0.mod_val);
	program_mmio(combo_phy, 1);
}

/**
* @brief
*	This function waits until the Combo programming is
*	finished. There is a timer for which time the new values will remain in
*	effect. After that timer expires, the original values will be restored.
* @param t - The timer which needs to be deleted
* @return void
*/
void combo::wait_until_done()
{
	TRACING();

	ddi_sel *ds = get_ds();
	combo_phy_reg *combo_phy = (combo_phy_reg *) ds->phy_data;

	// Wait to write back the original value
	while(!combo_phy->done) {
		usleep(1000);
	}
	timer_delete(get_timer());
}

/**
* @brief
* This function programs the Combo Phy MMIO registers
* needed to move avsync period for a system.
* @param *pr - The data structure that holds all of the PHY registers
* that need to be programmed
* @param mod - This parameter tells the function whether to program the original
* values or the modified ones.
* - 0 = Original
* - 1 = modified
* @return void
*/
void combo::program_mmio(combo_phy_reg *pr, int mod)
{
#if !TESTING
	WRITE_OFFSET_DWORD(pr->cfgcr0.addr,
			mod ? pr->cfgcr0.mod_val : pr->cfgcr0.orig_val);
#endif
}

/**
* @brief
* This function resets the Combo Phy MMIO registers to their
* original value. It gets executed whenever a timer expires. We program MMIO
* registers of the PHY in this function becase we have waited for a certain
* time period to get the primary and secondary systems vsync in sync and now
* it is time to reprogram the default values for the secondary system's PHYs.
* @param sig - The signal that fired
* @param *si - A pointer to a siginfo_t, which is a structure containing
* further information about the signal
* @param *uc - This is a pointer to a ucontext_t structure, cast to void *.
* The structure pointed to by this field contains signal context information
* that was saved on the user-space stack by the kernel
* @return void
*/
void combo::reset_phy_regs(int sig, siginfo_t *si, void *uc)
{
	user_info *ui = (user_info *) si->si_value.sival_ptr;
	if(!ui) {
		return;
	}

	DBG("timer done\n");
	combo_phy_reg *cr = (combo_phy_reg *) ui->get_reg();
	combo * c = (combo *) ui->get_type();
	c->program_mmio(cr, 0);
	DBG("DEFAULT VALUES\n cfgcr0 [0x%X] =\t 0x%X\n cfgcr1 [0x%X] =\t 0x%X\n",
			cr->cfgcr0.addr, cr->cfgcr0.orig_val, cr->cfgcr1.addr, cr->cfgcr1.orig_val);
	cr->done = 1;
	delete ui;
}
