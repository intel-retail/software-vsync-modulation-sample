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
#include "dp_m_n.h"

#define PIPEA_M1 0x60040
#define PIPEA_N1 0x60044
#define PIPEB_M1 0x61040
#define PIPEB_N1 0x61044
#define PIPEC_M1 0x62040
#define PIPEC_N1 0x62044
#define PIPED_M1 0x63040
#define PIPED_N1 0x63044

dp_m_n_phy_reg dp_m_n_table[] = {
	{REG(PIPEA_M1), REG(PIPEA_N1), 0},
	{REG(PIPEB_M1), REG(PIPEB_N1), 0},
	{REG(PIPEC_M1), REG(PIPEC_N1), 0},
	{REG(PIPED_M1), REG(PIPED_N1), 0},
};

/**
 * @brief Constructor for a new dp_m_n::dp_m_n
 *
 * @param ds
 * @param pipe
 */
dp_m_n::dp_m_n(ddi_sel* ds, int _pipe) : phys(_pipe)
{
	TRACING();
	dp_m_n_phy = {};

	if (!ds) {
		ERR("ddi_sel pointer is null\n");
		return;
	}

	// One PHY should be connected to one display only
	// Use pipe directly to figure out M&N Addresses
	dp_m_n_phy = &dp_m_n_table[get_pipe()];
	dp_m_n_phy->enabled = 1;
	set_ds(ds);
	set_init(true);
	done = 1;
	phy_type = M_N;

}

dp_m_n::~dp_m_n()
{
	dp_m_n_phy->enabled = 0;
}

/**
 * @brief
 * This function called by phy class and delegate calculation
 * to overloaded function.
 *
 * @return double - The calculated PLL clock
 */
double dp_m_n::calculate_pll_clock()
{
	TRACING();
	// Return M value as frequency.
	double freq = (double) dp_m_n_phy->mreg.orig_val;
	return freq;
}

/**
 * @brief
 * This function calculates the feedback dividers for the PLL frequency
 *
 * @param pll_clock - The desired PLL frequency
 * @return 0 - success, non zero - failure
 */
int dp_m_n::calculate_feedback_dividers(double pll_clock)
{
	TRACING();

	// Set given new pll_clock as new M value
	dp_m_n_phy->mreg.mod_val = (uint32_t) pll_clock;

	return 0;
}


/**
 * @brief
 * This function reads the Combo Phy M&N MMIO registers
 * @param None
 * @return void
 */
void dp_m_n::read_registers()
{
	TRACING();

	dp_m_n_phy->mreg.orig_val = READ_OFFSET_DWORD(dp_m_n_phy->mreg.addr);
	dp_m_n_phy->nreg.orig_val = READ_OFFSET_DWORD(dp_m_n_phy->nreg.addr);

	// Set modified values to original value.  Since N modified value will be the same.
	dp_m_n_phy->mreg.mod_val = dp_m_n_phy->mreg.orig_val;
	dp_m_n_phy->nreg.mod_val = dp_m_n_phy->nreg.orig_val;

}

/**
 * @brief
 * This function prints the register value
 *
 * @param None
 * @return void
 */
void dp_m_n::print_registers()
{
	TRACING();
	PRINT("Original Value ->\n");
	PRINT("\tM register[0x%X]= 0x%X, %d\n", dp_m_n_phy->mreg.addr, dp_m_n_phy->mreg.orig_val, dp_m_n_phy->mreg.orig_val);
	PRINT("\tN register[0x%X]= 0x%X, %d\n", dp_m_n_phy->nreg.addr, dp_m_n_phy->nreg.orig_val, dp_m_n_phy->nreg.orig_val);
	PRINT("Updated Value ->\n");
	PRINT("\tM register[0x%X]= 0x%X, %d\n", dp_m_n_phy->mreg.addr, dp_m_n_phy->mreg.mod_val, dp_m_n_phy->mreg.mod_val);
	PRINT("\tN register[0x%X]= 0x%X, %d\n", dp_m_n_phy->nreg.addr, dp_m_n_phy->nreg.mod_val, dp_m_n_phy->nreg.mod_val);

}

/**
* @brief
* This function programs the DP M & N registers
*
* @param mod - This parameter tells the function whether to program the original
* values or the modified ones.
* - 0 = Original
* - 1 = modified
* @return 0 - success, non zero - failure
*/
int dp_m_n::program_mmio(int mod)
{
	TRACING();

	// Update M register
	WRITE_OFFSET_DWORD(dp_m_n_phy->mreg.addr,
			mod ? dp_m_n_phy->mreg.mod_val : dp_m_n_phy->mreg.orig_val);

	// Update N register
	WRITE_OFFSET_DWORD(dp_m_n_phy->nreg.addr,
			mod ? dp_m_n_phy->nreg.mod_val :  dp_m_n_phy->nreg.orig_val);

	return 0;
}

