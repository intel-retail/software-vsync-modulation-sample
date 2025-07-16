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
#include <time.h>
#include "mmio.h"
#include "dkl.h"

dkl_phy_reg dkl_table[] = {
	{REG(DKL_PLL_DIV0(0)), REG(DKL_VISA_SERIALIZER(0)), REG(DKL_BIAS(0)), REG(DKL_SSC(0)), REG(DKL_DCO(0)), REG(HIP_INDEX_REG(0)),
		HIP_INDEX_VAL(0, 2), 0},
	{REG(DKL_PLL_DIV0(1)), REG(DKL_VISA_SERIALIZER(1)), REG(DKL_BIAS(1)), REG(DKL_SSC(1)), REG(DKL_DCO(1)), REG(HIP_INDEX_REG(1)),
		HIP_INDEX_VAL(1, 2), 0},
	{REG(DKL_PLL_DIV0(2)), REG(DKL_VISA_SERIALIZER(2)), REG(DKL_BIAS(2)), REG(DKL_SSC(2)), REG(DKL_DCO(2)), REG(HIP_INDEX_REG(2)),
		HIP_INDEX_VAL(2, 2), 0},
	{REG(DKL_PLL_DIV0(3)), REG(DKL_VISA_SERIALIZER(3)), REG(DKL_BIAS(3)), REG(DKL_SSC(3)), REG(DKL_DCO(3)), REG(HIP_INDEX_REG(3)),
		HIP_INDEX_VAL(3, 2), 0},
	{REG(DKL_PLL_DIV0(4)), REG(DKL_VISA_SERIALIZER(4)), REG(DKL_BIAS(4)), REG(DKL_SSC(4)), REG(DKL_DCO(4)), REG(HIP_INDEX_REG(4)),
		HIP_INDEX_VAL(4, 2), 0},
	{REG(DKL_PLL_DIV0(5)), REG(DKL_VISA_SERIALIZER(5)), REG(DKL_BIAS(5)), REG(DKL_SSC(5)), REG(DKL_DCO(5)), REG(HIP_INDEX_REG(5)),
		HIP_INDEX_VAL(5, 2), 0},
};

register_info reg_dkl_pll_div0 = { "DKL_PLL_DIV0",
							{{"i_fbdiv_intgr", 0, 7},  // 10 bits
							{"i_fbprediv_3_0", 8, 11}, // 15 bits
							{"", 0, 0}} };

register_info reg_dkl_bias = { "DKL_BIAS",
							{{"i_fbdivfrac_21_0", 8, 29},
							{"", 0, 0}} };

/**
 * @brief Construct a new dkl::dkl object
 *
 * @param ds
 * @param first_dkl_phy_loc
 */
dkl::dkl(ddi_sel* ds, int first_dkl_phy_loc, int _pipe) : phys(_pipe)
{
	TRACING();
	unsigned int val;
	dkl_phy = {};
	dpll_num = 0;

	if (!ds) {
		ERR("ddi_sel pointer is null\n");
		return;
	}

	if (ds->de_clk < first_dkl_phy_loc) {
		ERR("Invalid de_clk (0x%X) or the location of the first dkl phy (0x%X).\n", ds->de_clk, first_dkl_phy_loc);
		return;
	}
	dpll_num = ds->de_clk - first_dkl_phy_loc;
	val = READ_OFFSET_DWORD(dkl_table[ds->de_clk - first_dkl_phy_loc].dkl_pll_div0.addr);
	if (val != 0xFFFFFFFF) {
		// One PHY should be connected to one display only
		if (!dkl_table[dpll_num].enabled) {
			dkl_table[dpll_num].enabled = 1;
			dkl_phy = &dkl_table[dpll_num];
			set_ds(ds);
			set_init(true);
			done = 1;
		}
	}

	phy_type = DKL;
}

/**
 * @brief Destructor for Dekel PHY
 */
dkl::~dkl()
{
	dkl_table[dpll_num].enabled = 0;
}

/**
 * @brief
 * This function reads the DKL PHY registers
 * @param - None
 * @return void
 */
void dkl::read_registers()
{
	TRACING();
	dkl_phy->dkl_pll_div0.orig_val = dkl_phy->dkl_pll_div0.mod_val = READ_OFFSET_DWORD(dkl_phy->dkl_pll_div0.addr);
	if (!dkl_phy->dkl_pll_div0.orig_val) {
		WRITE_OFFSET_DWORD(dkl_phy->dkl_index.addr, dkl_phy->dkl_index_val);
		dkl_phy->dkl_pll_div0.orig_val = dkl_phy->dkl_pll_div0.mod_val = READ_OFFSET_DWORD(dkl_phy->dkl_pll_div0.addr);
	}

	dkl_phy->dkl_visa_serializer.orig_val = dkl_phy->dkl_visa_serializer.mod_val = READ_OFFSET_DWORD(dkl_phy->dkl_visa_serializer.addr);
	dkl_phy->dkl_bias.orig_val = dkl_phy->dkl_bias.mod_val = READ_OFFSET_DWORD(dkl_phy->dkl_bias.addr);
	dkl_phy->dkl_ssc.orig_val = dkl_phy->dkl_ssc.mod_val = READ_OFFSET_DWORD(dkl_phy->dkl_ssc.addr);
	dkl_phy->dkl_dco.orig_val = dkl_phy->dkl_dco.mod_val = READ_OFFSET_DWORD(dkl_phy->dkl_dco.addr);
}

/**
 * @brief
 * This function called by phy class and delegate calculation
 * to overloaded function based on the dkl_pll_div0 and dkl_bias values
 *
 * @param None
 * @return double - The calculated PLL clock
 */
double dkl::calculate_pll_clock()
{
	TRACING();
	uint32_t div0 = READ_OFFSET_DWORD(dkl_phy->dkl_pll_div0.addr);
	if (!div0) {
		WRITE_OFFSET_DWORD(dkl_phy->dkl_index.addr, dkl_phy->dkl_index_val);
		div0 = READ_OFFSET_DWORD(dkl_phy->dkl_pll_div0.addr);
	}
	uint32_t bias = READ_OFFSET_DWORD(dkl_phy->dkl_bias.addr);

	return calculate_pll_clock(div0, bias);
}

/**
 * @brief
 * This function calculates pll clock based on the dkl_pll_div0 and dkl_bias values
 *
 * @param dkl_pll_div0 - The dkl_pll_div0 register value
 * @param dkl_bias - The dkl_bias register value
 * @return double - The calculated PLL clock
 */
double dkl::calculate_pll_clock(uint32_t dkl_pll_div0, uint32_t dkl_bias)
{
	TRACING();

	// PLL frequency in MHz (base) = 38.4* DKL_PLL_DIV0[i_fbprediv_3_0] *
	//		( DKL_PLL_DIV0[i_fbdiv_intgr_7_0]  + DKL_BIAS[i_fbdivfrac_21_0] / 2^22 )
	int i_fbprediv_3_0 = GETBITS_VAL(dkl_pll_div0, 11, 8);
	int i_fbdiv_intgr_7_0 = GETBITS_VAL(dkl_pll_div0, 7, 0);
	int i_fbdivfrac_21_0 = GETBITS_VAL(dkl_bias, 29, 8);
	double pll_freq = (double)(REF_CLK_FREQ * i_fbprediv_3_0 * (i_fbdiv_intgr_7_0 + i_fbdivfrac_21_0 / pow(2, 22)));
	return pll_freq;
}

/**
 * @brief
 * This function called by phy class and delegate calculation
 * to overloaded function.
 *
 * @param pll_freq - The desired PLL frequency
 * @return 0 - success, non zero - failure
 */
int dkl::calculate_feedback_dividers(double pll_freq)
{
	TRACING();
	uint8_t new_i_fbdiv_intgr_7_0 = 0;
	uint32_t new_i_fbdivfrac_21_0 = 0;
	uint32_t div0 = READ_OFFSET_DWORD(dkl_phy->dkl_pll_div0.addr);

	if (!div0) {
		WRITE_OFFSET_DWORD(dkl_phy->dkl_index.addr, dkl_phy->dkl_index_val);
		div0 = READ_OFFSET_DWORD(dkl_phy->dkl_pll_div0.addr);
	}

	uint32_t bias = READ_OFFSET_DWORD(dkl_phy->dkl_bias.addr);

	calculate_feedback_dividers(pll_freq, div0, &new_i_fbdiv_intgr_7_0, &new_i_fbdivfrac_21_0);

	dkl_phy->dkl_pll_div0.mod_val &= ~GENMASK(7, 0);
	dkl_phy->dkl_pll_div0.mod_val |= new_i_fbdiv_intgr_7_0;
	dkl_phy->dkl_bias.mod_val &= ~GENMASK(29, 8);
	dkl_phy->dkl_bias.mod_val |= (long)new_i_fbdivfrac_21_0 << 8;
	dkl_phy->dkl_visa_serializer.mod_val &= ~GENMASK(2, 0);
	dkl_phy->dkl_visa_serializer.mod_val |= 0x200;
	dkl_phy->dkl_ssc.mod_val &= ~GENMASK(31, 29);
	dkl_phy->dkl_ssc.mod_val |= 0x2 << 29;
	dkl_phy->dkl_ssc.mod_val &= ~BIT(13);
	dkl_phy->dkl_ssc.mod_val |= 0x2 << 12;
	dkl_phy->dkl_dco.mod_val &= ~BIT(2);
	dkl_phy->dkl_dco.mod_val |= 0x2 << 1;

	DBG("\tdkl_pll_div0[0x%X]: 0x%X -> 0x%X\n", dkl_phy->dkl_pll_div0.addr, div0, dkl_phy->dkl_pll_div0.mod_val);
	DBG("\tdkl_visa_serializer[0x%X]: 0x%X -> 0x%X\n", dkl_phy->dkl_visa_serializer.addr, dkl_phy->dkl_visa_serializer.orig_val, dkl_phy->dkl_visa_serializer.mod_val);
	DBG("\tdkl_bias[0x%X]: 0x%X -> 0x%X\n", dkl_phy->dkl_bias.addr, bias, dkl_phy->dkl_bias.mod_val);
	DBG("\tdkl_ssc[0x%X]: 0x%X -> 0x%X\n", dkl_phy->dkl_ssc.addr, dkl_phy->dkl_ssc.orig_val, dkl_phy->dkl_ssc.mod_val);
	DBG("\tdkl_dco[0x%X]: 0x%X -> 0x%X\n", dkl_phy->dkl_dco.addr, dkl_phy->dkl_dco.orig_val, dkl_phy->dkl_dco.mod_val);

	return 0;
}

/**
 * @brief
 * Function to calculate i_fbdiv_intgr_7_0 and i_fbdivfrac_21_0
 *
 * @param pll_freq - The desired PLL frequency
 * @param dkl_pll_div0 - The dkl_pll_div0 register value.
 * @param i_fbdiv_intgr_7_0 - Return value: The integer part of the feedback divider
 * @param i_fbdivfrac_21_0 - Return value: The fractional part of the feedback divider
 */
void dkl::calculate_feedback_dividers(double pll_freq, uint32_t dkl_pll_div0,
	uint8_t* i_fbdiv_intgr_7_0, uint32_t* i_fbdivfrac_21_0)
{
	TRACING();
	const uint8_t max_int_part = 0xFF;
	const uint32_t max_frac_part = 0x3FFFFF;

	uint8_t i_fbprediv_3_0 = GETBITS_VAL(dkl_pll_div0, 11, 8);
	double F = pll_freq / (REF_CLK_FREQ * i_fbprediv_3_0);

	if (!i_fbdiv_intgr_7_0 || !i_fbdivfrac_21_0) {
		ERR("Invalid pointer\n");
		return;
	}
	// Calculate the integer part (8-bit, 0-0xFF)
	*i_fbdiv_intgr_7_0 = (uint8_t)fmin(floor(F), max_int_part);

	// Calculate the fractional part
	double fractional_part = F - (double)*i_fbdiv_intgr_7_0;

	// Calculate the 22-bit fractional part (0 - 0x3FFFFF)
	*i_fbdivfrac_21_0 = (uint32_t)round(fractional_part * pow(2, 22));

	if (*i_fbdivfrac_21_0 > max_frac_part) {
		*i_fbdivfrac_21_0 = max_frac_part;
	}
}

/**
 * @brief
 * This function prints the register value
 *
 * @param None
 * @return void
 */
void dkl::print_registers()
{
	TRACING();
	PRINT("Original Value ->\n");
	print_register(dkl_phy->dkl_pll_div0.orig_val, dkl_phy->dkl_pll_div0.addr, &reg_dkl_pll_div0);
	print_register(dkl_phy->dkl_bias.orig_val, dkl_phy->dkl_bias.addr, &reg_dkl_bias);
	PRINT("dkl_phy->dkl_visa_serializer=0x%X\n", dkl_phy->dkl_visa_serializer.orig_val);
	PRINT("dkl_phy->dkl_ssc=0x%X\n", dkl_phy->dkl_ssc.orig_val);
	PRINT("dkl_phy->dkl_dco=0x%X\n", dkl_phy->dkl_dco.orig_val);

	PRINT("Updated Value ->\n");
	print_register(dkl_phy->dkl_pll_div0.mod_val, dkl_phy->dkl_pll_div0.addr, &reg_dkl_pll_div0);
	print_register(dkl_phy->dkl_bias.mod_val, dkl_phy->dkl_bias.addr, &reg_dkl_bias);
	PRINT("dkl_phy->dkl_visa_serializer=0x%X\n", dkl_phy->dkl_visa_serializer.mod_val);
	PRINT("dkl_phy->dkl_ssc=0x%X\n", dkl_phy->dkl_ssc.mod_val);
	PRINT("dkl_phy->dkl_dco=0x%X\n", dkl_phy->dkl_dco.mod_val);
}

/**
* @brief
* This function programs the DKL Phy MMIO registers needed
* to move a vsync period for a system.
* @param *pr - The data structure that holds all of the PHY registers that
* need to be programmed
* @param mod - This parameter tells the function whether to program the original
* values or the modified ones.
* - 0 = Original
* - 1 = modified
* @return 0 - success, non zero - failure
*/
int dkl::program_mmio(int mod)
{
	TRACING();

	/*
	 * Each Dekel PHY is addressed through a 4KB aperture. Each PHY has more than
	 * 4KB of register space, so a separate index is programmed in HIP_INDEX_REG0
	 * or HIP_INDEX_REG1, based on the port number, to set the upper 2 address
	 * bits that point the 4KB window into the full PHY register space.
	 * So, basically, if we read the value of the PLL DIV0 register and it is 0,
	 * then that means that this aperture may need to be shifted. Once we do that,
	 * we should re-read the value of the DIV0 register once more and this time it
	 * should be valid.
	 */
	if (!READ_OFFSET_DWORD(dkl_phy->dkl_pll_div0.addr)) {
		WRITE_OFFSET_DWORD(dkl_phy->dkl_index.addr, dkl_phy->dkl_index_val);
	}

	WRITE_OFFSET_DWORD(dkl_phy->dkl_pll_div0.addr,
		mod ? dkl_phy->dkl_pll_div0.mod_val : dkl_phy->dkl_pll_div0.orig_val);
	WRITE_OFFSET_DWORD(dkl_phy->dkl_visa_serializer.addr,
		mod ? dkl_phy->dkl_visa_serializer.mod_val : dkl_phy->dkl_visa_serializer.orig_val);
	WRITE_OFFSET_DWORD(dkl_phy->dkl_bias.addr,
		mod ? dkl_phy->dkl_bias.mod_val : dkl_phy->dkl_bias.orig_val);
	WRITE_OFFSET_DWORD(dkl_phy->dkl_ssc.addr,
		mod ? dkl_phy->dkl_ssc.mod_val : dkl_phy->dkl_ssc.orig_val);
	WRITE_OFFSET_DWORD(dkl_phy->dkl_dco.addr,
		mod ? dkl_phy->dkl_dco.mod_val : dkl_phy->dkl_dco.orig_val);

	return 0;
}
