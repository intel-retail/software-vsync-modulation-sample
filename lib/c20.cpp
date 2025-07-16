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
#include <errno.h>
#include <time.h>
#include "mmio.h"
#include "c20.h"

 /**
  * @brief
  * Constructor for c20 class
  * @param ds
  * @param pipe
  */
c20::c20(ddi_sel* ds, int _pipe) : phys(_pipe)
{
	c20_reg = {};
	TRACING();
	if (!ds) {
		ERR("ddi_sel pointer is null\n");
		return;
	}

	memset(&c20_reg, 0, sizeof(c20_reg));
	ds->phy_data = &c20_reg;
	set_ds(ds);
	set_init(true);
	done = 1;
	phy_type = C20;
}

/**
 * @brief
 * This is an inline function to multiply two 32-bit numbers
 * and return the result as a 64-bit number.
 * @param a - The first 32-bit number
 * @param b - The second 32-bit number
 */
static inline unsigned long long mul_u32_u32(u32 a, u32 b)
{
	return (unsigned long long)a * b;
}

/**
 * @brief
 * This function called by base phy class and calculates the PLL
 * clock frequency for c20 PHY.
 * @param None
 * @return double - The calculated PLL clock
 */
double c20::calculate_pll_clock()
{
	TRACING();
	ddi_sel* ds = get_ds();
	if (!ds) {
		ERR("Invalid ddi_sel\n");
		return 0;
	}

	uint32_t frac_en = c20_reg.frac_en;
	uint32_t fb_clk_div4_en = c20_reg.fb_clk_div4_en;
	uint32_t tx_rate_mult = c20_reg.tx_rate_mult;
	uint32_t tx_clk_div = c20_reg.tx_clk_div;
	uint32_t tx_rate = c20_reg.tx_rate;
	uint32_t ref_clk_mpllb_div = c20_reg.ref_clk_mpllb_div;
	uint32_t multiplier = c20_reg.multiplier;
	uint32_t frac_quot = c20_reg.frac_quot;
	uint32_t frac_rem = c20_reg.frac_rem;
	uint32_t frac_den = c20_reg.frac_den;

	double frac = 0.0;
	if (frac_en && frac_den != 0) {
		frac = frac_quot + ((double)frac_rem / frac_den);
	}

	double refclk = REF_CLK_FREQ * 1000.0;
	double ref = (refclk * (1 << (1 + fb_clk_div4_en))) / (1 << ref_clk_mpllb_div);

	// Calculate the VCO (Voltage Controlled Oscillator) frequency.
	// The VCO frequency is derived by multiplying the reference clock frequency (ref)
	// with the sum of the integer part of the multiplier (shifted left by 15 bits) and
	// the fractional part (frac). The result is then right-shifted by 17 bits to scale
	// it down, and finally divided by 10 to get the final VCO frequency.

	double vco_numerator = ref * ((multiplier * 1.0 * (1 << 15)) + frac);
	double vco = (vco_numerator / (1 << 17)) / 10.0;

	double final_freq = vco / (1 << tx_rate);
	final_freq = final_freq / (1 << tx_clk_div);
	final_freq = final_freq * (1 << tx_rate_mult);

	return final_freq;
}

/**
 * @brief
 * This function calculates the feedback dividers for the PLL frequency
 * and store the values in the c20_reg.pll_state_mod array
 * @param pll_freq - The desired PLL frequency
 * @return 0 - success, non zero - failure
 */
int c20::calculate_feedback_dividers(double pll_freq)
{
	TRACING();

	uint32_t frac_den = c20_reg.frac_den;
	uint32_t fb_clk_div4_en = c20_reg.fb_clk_div4_en;
	uint32_t tx_rate_mult = c20_reg.tx_rate_mult;
	uint32_t tx_clk_div = c20_reg.tx_clk_div;
	uint32_t tx_rate = c20_reg.tx_rate;
	uint32_t ref_clk_mpllb_div = c20_reg.ref_clk_mpllb_div;
	uint32_t multiplier = c20_reg.multiplier;
	double refclk = REF_CLK_FREQ * 1000;
	double ref = (refclk * (1 << (1 + fb_clk_div4_en))) / (1 << ref_clk_mpllb_div);

	double vco = pll_freq;
	vco = vco * (1 << tx_rate);
	vco = vco * (1 << tx_clk_div);
	vco = vco / (1 << tx_rate_mult);

	double frac_scaled = ((vco * 10.0 * (1 << 17)) / ref) - (multiplier << (17 - 2));
	uint32_t frac_quot_mod = (uint32_t)round(frac_scaled);   // Take the integer part
	uint32_t frac_rem_mod  = (uint32_t)round((frac_scaled - frac_quot_mod) * frac_den);  // Compute remainder
	c20_reg.pll_state_mod[RAWCMN_DIG_CFG_INDEX_8] = frac_quot_mod ;

	// Ensure the remainder is within the valid range [0, 65535]
	if (frac_rem_mod > 65535) {
		DBG("Remainder exceeds maximum value, adjusting to 65535\n");
		frac_rem_mod = 65535;
	}

	c20_reg.pll_state_mod[RAWCMN_DIG_CFG_INDEX_9] = frac_rem_mod ;

	INFO("\tmpll_fracn_quot: 0x%X [%d] -> 0x%X [%d]\n",
			(int)c20_reg.frac_quot, (int)c20_reg.frac_quot, frac_quot_mod, frac_quot_mod);
	INFO("\tmpll_fracn_rem: 0x%X [%d] -> 0x%X [%d]\n",
			c20_reg.frac_rem, c20_reg.frac_rem, (int)frac_rem_mod, (int)frac_rem_mod);

	return 0;
}

/**
 * @brief
 * This function prints the register value
 * @param None
 * @return void
 */
void c20::print_registers()
{
	TRACING();

	PRINT("Original Value ->\n");
	for (int i = 0; i < C20_PLL_REG_COUNT; i++) {
		PRINT("c20: pll_state[%d] = 0x%X, %d\n", i, c20_reg.pll_state_orig[i], c20_reg.pll_state_orig[i]);
	}
	PRINT("Updated Value ->\n");
	for (int i = 0; i < C20_PLL_REG_COUNT; i++) {
		PRINT("c20: pll_state[%d] = 0x%X, %d\n", i, c20_reg.pll_state_mod[i], c20_reg.pll_state_mod[i]);
	}
}

/**
 * @brief
 * This function reads the c20 Phy MMIO registers
 * @param None
 * @return void
 */
void c20::read_registers()
{
	TRACING();
	ddi_sel* ds = get_ds();
	if (!ds) {
		ERR("Invalid ddi_sel\n");
		return;
	}

	u32 port = ds->de_clk - 1;

	bool cntx = intel_cx0_read(port, INTEL_CX0_LANE0, PHY_C20_VDR_CUSTOM_SERDES_RATE) & PHY_C20_CONTEXT_TOGGLE;
	/* Read Tx configuration */
	for (int i = 0; i < ARRAY_SIZE(c20_reg.tx); i++) {
		if (cntx)
			c20_reg.tx[i] = intel_c20_sram_read(port, INTEL_CX0_LANE0,
				PHY_C20_B_TX_CNTX_CFG(i));
		else
			c20_reg.tx[i] = intel_c20_sram_read(port, INTEL_CX0_LANE0,
				PHY_C20_A_TX_CNTX_CFG(i));

		DBG("Tx[%d] = %d [0x%x]\n", i, c20_reg.tx[i], c20_reg.tx[i]);
		c20_reg.tx_rate = REG_FIELD_GET(C20_PHY_TX_RATE, c20_reg.tx[0]);
	}

	/* Read common configuration */
	for (int i = 0; i < ARRAY_SIZE(c20_reg.cmn); i++) {
		if (cntx)
			c20_reg.cmn[i] = intel_c20_sram_read(port, INTEL_CX0_LANE0,
				PHY_C20_B_CMN_CNTX_CFG(i));
		else
			c20_reg.cmn[i] = intel_c20_sram_read(port, INTEL_CX0_LANE0,
				PHY_C20_A_CMN_CNTX_CFG(i));

		DBG("cmn[%d]=%d\n", i, c20_reg.cmn[i]);
	}

	// Read the PLL configuration
	// Depending on the context (cntx) and MPLL type (A or B), read the PLL
	// configuration from either the B or A context configuration registers.
	// The read values are stored in the c20_reg.pll_state_orig array.
	// The same values are also copied to the c20_reg.pll_state_mod array for
	// potential modification later.  Some configuration are depending on
	// PLL A or B that's why variables (e.g frac_quot) are updated here which
	// to be used later in calculations.
	if (c20_reg.tx[0] & C20_PHY_USE_MPLLB) {
		/* MPLLB configuration */
		for (int i = 0; i < ARRAY_SIZE(c20_reg.pll_state_orig); i++) {
			if (cntx)
				c20_reg.pll_state_orig[i] = intel_c20_sram_read(port, INTEL_CX0_LANE0,
					PHY_C20_B_MPLLB_CNTX_CFG(i));
			else
				c20_reg.pll_state_orig[i] = intel_c20_sram_read(port, INTEL_CX0_LANE0,
					PHY_C20_A_MPLLB_CNTX_CFG(i));

			c20_reg.pll_state_mod[i] = c20_reg.pll_state_orig[i];
			DBG("MPLLB[%d] = %d [0x%x]\n", i, c20_reg.pll_state_orig[i], c20_reg.pll_state_orig[i]);
		}
		// Read the fractional quotient and remainder values directly from sram
		c20_reg.pll_state_mod[RAWCMN_DIG_CFG_INDEX_8] =
						c20_reg.pll_state_orig[RAWCMN_DIG_CFG_INDEX_8] =
						intel_c20_sram_read(port, INTEL_CX0_LANE0, RAWCMN_DIG_MPLLB_CNTX_CFG_8);

		c20_reg.pll_state_mod[RAWCMN_DIG_CFG_INDEX_9] =
						c20_reg.pll_state_orig[RAWCMN_DIG_CFG_INDEX_9] =
						intel_c20_sram_read(port, INTEL_CX0_LANE0, RAWCMN_DIG_MPLLB_CNTX_CFG_9);

		c20_reg.tx_rate_mult = C20_TX_RATE_MULT;
		c20_reg.fb_clk_div4_en = C20_CLK_DIV4_EN;
		c20_reg.frac_en = REG_FIELD_GET(C20_MPLLB_FRACEN, c20_reg.pll_state_orig[RAWCMN_DIG_CFG_INDEX_6]);
		c20_reg.frac_quot = c20_reg.pll_state_orig[RAWCMN_DIG_CFG_INDEX_8];
		c20_reg.frac_rem = c20_reg.pll_state_orig[RAWCMN_DIG_CFG_INDEX_9];
		c20_reg.frac_den = c20_reg.pll_state_orig[RAWCMN_DIG_CFG_INDEX_7];
		c20_reg.multiplier = REG_FIELD_GET(C20_MULTIPLIER_MASK, c20_reg.pll_state_orig[RAWCMN_DIG_CFG_INDEX_0]);
		c20_reg.tx_clk_div = REG_FIELD_GET(C20_MPLLB_TX_CLK_DIV_MASK, c20_reg.pll_state_orig[RAWCMN_DIG_CFG_INDEX_0]);
		c20_reg.ref_clk_mpllb_div = REG_FIELD_GET(C20_REF_CLK_MPLLB_DIV_MASK, c20_reg.pll_state_orig[RAWCMN_DIG_CFG_INDEX_6]);

	}
	else {
		/* MPLLA configuration */
		for (int i = 0; i < ARRAY_SIZE(c20_reg.pll_state_orig); i++) {
			if (cntx)
				c20_reg.pll_state_orig[i] = intel_c20_sram_read(port, INTEL_CX0_LANE0,
					PHY_C20_B_MPLLA_CNTX_CFG(i));
			else
				c20_reg.pll_state_orig[i] = intel_c20_sram_read(port, INTEL_CX0_LANE0,
					PHY_C20_A_MPLLA_CNTX_CFG(i));

			c20_reg.pll_state_mod[i] = c20_reg.pll_state_orig[i];
			DBG("MPLLA[%d] = %d [0x%x]\n", i, c20_reg.pll_state_orig[i], c20_reg.pll_state_orig[i]);
		}

		// Read the fractional quotient and remainder values directly from sram
		c20_reg.pll_state_mod[RAWCMN_DIG_CFG_INDEX_8] =
						c20_reg.pll_state_orig[RAWCMN_DIG_CFG_INDEX_8] =
						intel_c20_sram_read(port, INTEL_CX0_LANE0, RAWCMN_DIG_MPLLA_CNTX_CFG_8);

		c20_reg.pll_state_mod[RAWCMN_DIG_CFG_INDEX_9] =
						c20_reg.pll_state_orig[RAWCMN_DIG_CFG_INDEX_9] =
						intel_c20_sram_read(port, INTEL_CX0_LANE0, RAWCMN_DIG_MPLLA_CNTX_CFG_9);

		c20_reg.tx_rate_mult = C20_TX_RATE_MULT;
		c20_reg.frac_en = REG_FIELD_GET(C20_MPLLA_FRACEN, c20_reg.pll_state_orig[RAWCMN_DIG_CFG_INDEX_6]);
		c20_reg.multiplier = REG_FIELD_GET(C20_MULTIPLIER_MASK, c20_reg.pll_state_orig[RAWCMN_DIG_CFG_INDEX_0]);
		c20_reg.tx_clk_div = REG_FIELD_GET(C20_MPLLA_TX_CLK_DIV_MASK, c20_reg.pll_state_orig[RAWCMN_DIG_CFG_INDEX_1]);
		c20_reg.ref_clk_mpllb_div = REG_FIELD_GET(C20_REF_CLK_MPLLB_DIV_MASK, c20_reg.pll_state_orig[RAWCMN_DIG_CFG_INDEX_6]);
		c20_reg.fb_clk_div4_en = REG_FIELD_GET(C20_FB_CLK_DIV4_EN, c20_reg.pll_state_orig[RAWCMN_DIG_CFG_INDEX_0]);
	}
}

/**
 * @brief
 * This function programs the c20 Phy MMIO registers
 * needed to move avsync period for a system.
 * @param mod - This parameter tells the function whether to program the original
 * values or the modified ones.
 * - 0 = Original
 * - 1 = modified
 * @return 0 - success, non zero - failure
 */
int c20::program_mmio(int mod)
{
	TRACING();

	ddi_sel* ds = get_ds();

	u32 port = ds->de_clk - 1;

	if (c20_reg.tx[0] & C20_PHY_USE_MPLLB) {
		intel_c20_sram_write(port, INTEL_CX0_LANE0, RAWCMN_DIG_MPLLB_CNTX_CFG_8,
			mod ? c20_reg.pll_state_mod[RAWCMN_DIG_CFG_INDEX_8] : c20_reg.pll_state_orig[RAWCMN_DIG_CFG_INDEX_8]);

		intel_c20_sram_write(port, INTEL_CX0_LANE0, RAWCMN_DIG_MPLLB_CNTX_CFG_9,
			mod ? c20_reg.pll_state_mod[RAWCMN_DIG_CFG_INDEX_9] : c20_reg.pll_state_orig[RAWCMN_DIG_CFG_INDEX_9]);

		intel_c20_sram_write(port, INTEL_CX0_LANE0, RAWCMN_DIG_MPLLB_FRAC_UPDATE, 0x1);
	}
	else {
		intel_c20_sram_write(port, INTEL_CX0_LANE0, RAWCMN_DIG_MPLLA_CNTX_CFG_8,
			mod ? c20_reg.pll_state_mod[RAWCMN_DIG_CFG_INDEX_8] : c20_reg.pll_state_orig[RAWCMN_DIG_CFG_INDEX_8]);

		intel_c20_sram_write(port, INTEL_CX0_LANE0, RAWCMN_DIG_MPLLA_CNTX_CFG_9,
			mod ? c20_reg.pll_state_mod[RAWCMN_DIG_CFG_INDEX_9] : c20_reg.pll_state_orig[RAWCMN_DIG_CFG_INDEX_9]);

		intel_c20_sram_write(port, INTEL_CX0_LANE0, RAWCMN_DIG_MPLLA_FRAC_UPDATE, 0x1);
	}

	return 0;
}
