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
#include "c10.h"

 /**
  * @brief
  * Constructor for c10 class
  * @param ds
  * @param pipe
  */
c10::c10(ddi_sel* ds, int _pipe) : phys(_pipe)
{
	TRACING();
	if (!ds) {
		ERR("ddi_sel pointer is null\n");
		return;
	}

	memset(&c10_reg, 0, sizeof(c10_reg));
	ds->phy_data = &c10_reg;
	set_ds(ds);
	set_init(true);
	done = 1;
	u32 port = ds->dpll_num - 1;
	INFO("c10: port = %d\n", port);
	phy_type = C10;
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
 * clock frequency for C10 PHY.
 * @param None
 * @return double - The calculated PLL clock
 */
double c10::calculate_pll_clock()
{
	TRACING();
	ddi_sel* ds = get_ds();
	if (!ds) {
		ERR("Invalid ddi_sel\n");
		return 0;
	}

	uint32_t mpll_frac_rem_15_0, mpll_frac_den_15_0, mpll_frac_quot_15_0, mpll_multiplier_11_0, ref_clk_mpll_div_2_0;
	mpll_frac_den_15_0 = c10_reg.pll_state_orig[C10_PLL_REG_DEN_HIGH] << 8 | c10_reg.pll_state_orig[C10_PLL_REG_DEN_LOW];
	mpll_frac_quot_15_0 = c10_reg.pll_state_orig[C10_PLL_REG_QUOT_HIGH] << 8 | c10_reg.pll_state_orig[C10_PLL_REG_QUOT_LOW];
	mpll_frac_rem_15_0 = c10_reg.pll_state_orig[C10_PLL_REG_REM_HIGH] << 8 | c10_reg.pll_state_orig[C10_PLL_REG_REM_LOW];
	mpll_multiplier_11_0 = (REG_FIELD_GET8(C10_PLL3_MULTIPLIERH_MASK, c10_reg.pll_state_orig[C10_PLL_REG_MULTIPLIER]) << 8 |
		c10_reg.pll_state_orig[2]) / 2 + 16;
	ref_clk_mpll_div_2_0 = REG_FIELD_GET8(C10_PLL15_TXCLKDIV_MASK, c10_reg.pll_state_orig[C10_PLL_REG_TXCLKDIV]);

	const double ref_clk_freq_khz = REF_CLK_FREQ * 1000;
	unsigned long long numenator = mul_u32_u32(ref_clk_freq_khz, (mpll_multiplier_11_0 << 16) + mpll_frac_quot_15_0) +
		DIV_ROUND_CLOSEST(ref_clk_freq_khz * mpll_frac_rem_15_0, mpll_frac_den_15_0);
	unsigned long long denominator = 10 << (ref_clk_mpll_div_2_0 + 16);
	double pll_freq = numenator/(double)denominator;
	return pll_freq;
}

/**
 * @brief
 * This function calculates the feedback dividers for the PLL frequency
 * and store the values in the c10_reg.pll_state_mod array
 * @param pll_freq - The desired PLL frequency
 * @return 0 - success, non zero - failure
 */
int c10::calculate_feedback_dividers(double pll_freq)
{
	TRACING();
	const double ref_clk_freq_khz = REF_CLK_FREQ * 1000;
	uint32_t ref_clk_mpll_div_2_0 = REG_FIELD_GET8(C10_PLL15_TXCLKDIV_MASK, c10_reg.pll_state_orig[C10_PLL_REG_TXCLKDIV]);
	uint32_t mpll_frac_rem_15_0 = c10_reg.pll_state_orig[C10_PLL_REG_REM_HIGH] << 8 | c10_reg.pll_state_orig[C10_PLL_REG_REM_LOW];
	uint32_t mpll_frac_den_15_0 = c10_reg.pll_state_orig[C10_PLL_REG_DEN_HIGH] << 8 | c10_reg.pll_state_orig[C10_PLL_REG_DEN_LOW];
	uint32_t mpll_multiplier_11_0 = (REG_FIELD_GET8(C10_PLL3_MULTIPLIERH_MASK, c10_reg.pll_state_orig[C10_PLL_REG_MULTIPLIER]) << 8 | c10_reg.pll_state_orig[2]) / 2 + 16;
	unsigned long long temp = pll_freq * (10 << (ref_clk_mpll_div_2_0 + 16));
	temp -= DIV_ROUND_CLOSEST_ULL(ref_clk_freq_khz * mpll_frac_rem_15_0, mpll_frac_den_15_0);
	temp /= (ref_clk_freq_khz);
	temp -= ((unsigned long long)mpll_multiplier_11_0 << 16);
	int new_mpll_frac_quot_15_0 = (int)temp;
	c10_reg.pll_state_mod[C10_PLL_REG_QUOT_LOW] = new_mpll_frac_quot_15_0 & GENMASK(7, 0);
	c10_reg.pll_state_mod[C10_PLL_REG_QUOT_HIGH] = new_mpll_frac_quot_15_0 >> 8;

	INFO("\tQuot Low: 0x%X [%d] -> 0x%X [%d]\n",
		c10_reg.pll_state_orig[C10_PLL_REG_QUOT_LOW], c10_reg.pll_state_orig[C10_PLL_REG_QUOT_LOW],
		(int)c10_reg.pll_state_mod[C10_PLL_REG_QUOT_LOW], (int)c10_reg.pll_state_mod[C10_PLL_REG_QUOT_LOW]);
	INFO("\tQuot High: 0x%X [%d] -> 0x%X [%d]\n",
		c10_reg.pll_state_orig[C10_PLL_REG_QUOT_HIGH], c10_reg.pll_state_orig[C10_PLL_REG_QUOT_HIGH],
		(int)c10_reg.pll_state_mod[C10_PLL_REG_QUOT_HIGH], (int)c10_reg.pll_state_mod[C10_PLL_REG_QUOT_HIGH]);

	return 0;
}

/**
 * @brief
 * This function prints the register value
 * @param None
 * @return void
 */
void c10::print_registers()
{
	TRACING();
	PRINT("Original Value ->\n");
	for (int i = 0; i < C10_PLL_REG_COUNT; i++) {
		PRINT("c10: pll_state[%d] = 0x%X, %d\n", i,
			static_cast<unsigned int>(c10_reg.pll_state_orig[i]),
			static_cast<int>(c10_reg.pll_state_orig[i]));
	}
	PRINT("Updated Value ->\n");
	for (int i = 0; i < C10_PLL_REG_COUNT; i++) {
		PRINT("c10: pll_state[%d] = 0x%X, %d\n", i,
			static_cast<unsigned int>(c10_reg.pll_state_mod[i]),
			static_cast<int>(c10_reg.pll_state_mod[i]));
	}
}

/**
 * @brief
 * This function reads the C10 Phy MMIO registers
 * @param None
 * @return void
 */
void c10::read_registers()
{
	TRACING();
	ddi_sel* ds = get_ds();
	if (!ds) {
		ERR("Invalid ddi_sel\n");
		return;
	}

	/*
	According to BSpecs, MTL and PTL DDIs for C10 correspond to ports A and B.
	Consequently, it is reasonable to deduce the port number from de_clk as clk-1.
	However, this assumption might not hold for future platforms, where it
	may be necessary to derive the port number directly from register values.
	For guidance on this approach, refer to combo.cpp.

	BSpecs:
	Port    Usage     Capability    Alternate Names
	DDI A | Port A  | eDP, DP, HDMI DDIA, Port A
	DDI B | Port B  | eDP, DP, HDMI DDIB, Port B
	*/

	// dpll_num holds port number.
	// MTL has port number same as ddi_select.  However PTL is different.
	u32 port = ds->dpll_num - 1;
	int pll_state[C10_PLL_REG_COUNT] = { 0 };
	for (int i = 0; i < C10_PLL_REG_COUNT; i++) {
		pll_state[i] = intel_cx0_read(port, INTEL_CX0_LANE0, PHY_C10_VDR_PLL(i));
		c10_reg.pll_state_orig[i] = c10_reg.pll_state_mod[i] = pll_state[i];
		DBG("c10: pll_state[%d] = 0x%X, %d\n", i, pll_state[i], pll_state[i]);
	}
}

/**
 * @brief
 * This function programs the C10 Phy MMIO registers
 * needed to move avsync period for a system.
 * @param mod - This parameter tells the function whether to program the original
 * values or the modified ones.
 * - 0 = Original
 * - 1 = modified
 * @return 0 - success, non zero - failure
 */
int c10::program_mmio(int mod)
{
	TRACING();
	ddi_sel* ds = get_ds();

	u32 port = ds->dpll_num - 1;

	intel_cx0_rmw(port, INTEL_CX0_BOTH_LANES, PHY_C10_VDR_CONTROL(1),
		0, C10_VDR_CTRL_MSGBUS_ACCESS,
		MB_WRITE_COMMITTED);

	/* Custom width needs to be programmed to 0 for both the phy lanes */
	intel_cx0_rmw(port, INTEL_CX0_BOTH_LANES, PHY_C10_VDR_CUSTOM_WIDTH,
		C10_VDR_CUSTOM_WIDTH_MASK, C10_VDR_CUSTOM_WIDTH_8_10,
		MB_WRITE_COMMITTED);
	intel_cx0_rmw(port, INTEL_CX0_BOTH_LANES, PHY_C10_VDR_CONTROL(1),
		0, C10_VDR_CTRL_UPDATE_CFG,
		MB_WRITE_COMMITTED);


	intel_cx0_write(port, INTEL_CX0_LANE0, PHY_C10_VDR_PLL(C10_PLL_REG_QUOT_LOW),
		mod ? c10_reg.pll_state_mod[C10_PLL_REG_QUOT_LOW] : c10_reg.pll_state_orig[C10_PLL_REG_QUOT_LOW],
		MB_WRITE_COMMITTED);

	intel_cx0_write(port, INTEL_CX0_LANE0, PHY_C10_VDR_PLL(C10_PLL_REG_QUOT_HIGH),
		mod ? c10_reg.pll_state_mod[C10_PLL_REG_QUOT_HIGH] : c10_reg.pll_state_orig[C10_PLL_REG_QUOT_HIGH],
		MB_WRITE_COMMITTED);

	intel_cx0_rmw(port, INTEL_CX0_LANE0, PHY_C10_VDR_CONTROL(1),
		0, C10_VDR_CTRL_MASTER_LANE | C10_VDR_CTRL_UPDATE_CFG,
		MB_WRITE_COMMITTED);

	return 0;
}
