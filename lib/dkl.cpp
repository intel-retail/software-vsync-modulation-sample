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
		HIP_INDEX_VAL(0, 2), 0, 1},
	{REG(DKL_PLL_DIV0(1)), REG(DKL_VISA_SERIALIZER(1)), REG(DKL_BIAS(1)), REG(DKL_SSC(1)), REG(DKL_DCO(1)), REG(HIP_INDEX_REG(1)),
		HIP_INDEX_VAL(1, 2), 0, 1},
	{REG(DKL_PLL_DIV0(2)), REG(DKL_VISA_SERIALIZER(2)), REG(DKL_BIAS(2)), REG(DKL_SSC(2)), REG(DKL_DCO(2)), REG(HIP_INDEX_REG(2)),
		HIP_INDEX_VAL(2, 2), 0, 1},
	{REG(DKL_PLL_DIV0(3)), REG(DKL_VISA_SERIALIZER(3)), REG(DKL_BIAS(3)), REG(DKL_SSC(3)), REG(DKL_DCO(3)), REG(HIP_INDEX_REG(3)),
		HIP_INDEX_VAL(3, 2), 0, 1},
	{REG(DKL_PLL_DIV0(4)), REG(DKL_VISA_SERIALIZER(4)), REG(DKL_BIAS(4)), REG(DKL_SSC(4)), REG(DKL_DCO(4)), REG(HIP_INDEX_REG(4)),
		HIP_INDEX_VAL(4, 2), 0, 1},
	{REG(DKL_PLL_DIV0(5)), REG(DKL_VISA_SERIALIZER(5)), REG(DKL_BIAS(5)), REG(DKL_SSC(5)), REG(DKL_DCO(5)), REG(HIP_INDEX_REG(5)),
		HIP_INDEX_VAL(5, 2), 0, 1},
};

dkl::dkl(ddi_sel *ds, int first_dkl_phy_loc)
{
	unsigned int val;

	if(ds->de_clk < first_dkl_phy_loc) {
		ERR("Invalid de_clk (0x%X) or the location of the first dkl phy (0x%X).\n", ds->de_clk, first_dkl_phy_loc);
		return;
	}

	val = READ_OFFSET_DWORD(dkl_table[ds->de_clk - first_dkl_phy_loc].dkl_pll_div0.addr);
	if(val != 0xFFFFFFFF) {
		// One PHY should be connected to one display only
		if(!dkl_table[ds->de_clk - first_dkl_phy_loc].enabled) {
			dkl_table[ds->de_clk - first_dkl_phy_loc].enabled = 1;
			ds->phy_data = &dkl_table[ds->de_clk - first_dkl_phy_loc];
			set_ds(ds);
			set_init(true);
		}
	}
}


/**
* @brief
* This function programs DKL phys on the system
* @param time_diff - This is the time difference in between the primary and the
* secondary systems in ms. If master is ahead of the slave , then the time
* difference is a positive number otherwise negative.
* @param *t - A pointer to a pointer where we need to store the timer
* @return void
*/
void dkl::program_phy(double time_diff)
{
	double shift = SHIFT;
	ddi_sel *ds = get_ds();
	dkl_phy_reg *dkl_phy = (dkl_phy_reg *) ds->phy_data;

#if TESTING
	dkl_phy->dkl_pll_div0.orig_val = 0x50284274;
	dkl_phy->dkl_visa_serializer.orig_val = 0x54321000;
	dkl_phy->dkl_bias.orig_val = 0XC1000000;
	dkl_phy->dkl_ssc.orig_val = 0x400020ff;
	dkl_phy->dkl_dco.orig_val = 0xe4004080;
#else
	dkl_phy->dkl_pll_div0.orig_val         = dkl_phy->dkl_pll_div0.mod_val        = READ_OFFSET_DWORD(dkl_phy->dkl_pll_div0.addr);

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
	if(!dkl_phy->dkl_pll_div0.orig_val) {
		WRITE_OFFSET_DWORD(dkl_phy->dkl_index.addr, dkl_phy->dkl_index_val);
		dkl_phy->dkl_pll_div0.orig_val         = dkl_phy->dkl_pll_div0.mod_val        = READ_OFFSET_DWORD(dkl_phy->dkl_pll_div0.addr);
	}

	dkl_phy->dkl_visa_serializer.orig_val  = dkl_phy->dkl_visa_serializer.mod_val = READ_OFFSET_DWORD(dkl_phy->dkl_visa_serializer.addr);
	dkl_phy->dkl_bias.orig_val             = dkl_phy->dkl_bias.mod_val            = READ_OFFSET_DWORD(dkl_phy->dkl_bias.addr);
	dkl_phy->dkl_ssc.orig_val              = dkl_phy->dkl_ssc.mod_val             = READ_OFFSET_DWORD(dkl_phy->dkl_ssc.addr);
	dkl_phy->dkl_dco.orig_val              = dkl_phy->dkl_dco.mod_val             = READ_OFFSET_DWORD(dkl_phy->dkl_dco.addr);
	// For whichever PHY we find, let's set the done flag to 0 so that we can later
	// have a timer for it to reset the default values back in their registers
	dkl_phy->done = 0;

	if(time_diff < 0) {
		shift *= -1;
	}
	int steps = CALC_STEPS_TO_SYNC(time_diff, shift);
	DBG("steps are %d\n", steps);
	user_info *ui = new user_info(this, dkl_phy);
	make_timer((long) steps, ui, reset_phy_regs);
#endif
	DBG("OLD VALUES\n dkl_pll_div0 [0x%X] =\t 0x%X\n dkl_visa_serializer [0x%X] =\t 0x%X\n "
			"dkl_bias [0x%X] =\t 0x%X\n dkl_ssc [0x%X] =\t 0x%X\n dkl_dco [0x%X] =\t 0x%X\n",
			dkl_phy->dkl_pll_div0.addr,
			dkl_phy->dkl_pll_div0.orig_val,
			dkl_phy->dkl_visa_serializer.addr,
			dkl_phy->dkl_visa_serializer.orig_val,
			dkl_phy->dkl_bias.addr,
			dkl_phy->dkl_bias.orig_val,
			dkl_phy->dkl_ssc.addr,
			dkl_phy->dkl_ssc.orig_val,
			dkl_phy->dkl_dco.addr,
			dkl_phy->dkl_dco.orig_val);

	// PLL frequency in MHz (base) = 38.4* DKL_PLL_DIV0[i_fbprediv_3_0] *
	//		( DKL_PLL_DIV0[i_fbdiv_intgr_7_0]  + DKL_BIAS[i_fbdivfrac_21_0] / 2^22 )
	int i_fbprediv_3_0    = GETBITS_VAL(dkl_phy->dkl_pll_div0.orig_val, 11, 8);
	int i_fbdiv_intgr_7_0 = GETBITS_VAL(dkl_phy->dkl_pll_div0.orig_val, 7, 0);
	int i_fbdivfrac_21_0  = GETBITS_VAL(dkl_phy->dkl_bias.orig_val, 29, 8);
	double pll_freq = (double) (REF_DKL_FREQ * i_fbprediv_3_0 * (i_fbdiv_intgr_7_0 + i_fbdivfrac_21_0 / pow(2, 22)));
	double new_pll_freq = pll_freq + (shift * pll_freq / 100);
	double new_i_fbdivfrac_21_0 = ((new_pll_freq / (REF_DKL_FREQ * i_fbprediv_3_0)) - i_fbdiv_intgr_7_0) * pow(2,22);

	if(new_i_fbdivfrac_21_0 < 0) {
		i_fbdiv_intgr_7_0 -= 1;
		dkl_phy->dkl_pll_div0.mod_val &= ~GENMASK(7, 0);
		dkl_phy->dkl_pll_div0.mod_val |= i_fbdiv_intgr_7_0;
		new_i_fbdivfrac_21_0 = ((new_pll_freq / (REF_DKL_FREQ * i_fbprediv_3_0)) - (i_fbdiv_intgr_7_0)) * pow(2,22);
	}

	DBG("old pll_freq \t %f\n", pll_freq);
	DBG("new_pll_freq \t %f\n", new_pll_freq);
	DBG("new fbdivfrac \t 0x%X\n", (int) new_i_fbdivfrac_21_0);

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

	DBG("NEW VALUES\n dkl_pll_div0 [0x%X] =\t 0x%X\n dkl_visa_serializer [0x%X] =\t 0x%X\n "
			"dkl_bias [0x%X] =\t 0x%X\n dkl_ssc [0x%X] =\t 0x%X\n dkl_dco [0x%X] =\t 0x%X\n",
			dkl_phy->dkl_pll_div0.addr,
			dkl_phy->dkl_pll_div0.mod_val,
			dkl_phy->dkl_visa_serializer.addr,
			dkl_phy->dkl_visa_serializer.mod_val,
			dkl_phy->dkl_bias.addr,
			dkl_phy->dkl_bias.mod_val,
			dkl_phy->dkl_ssc.addr,
			dkl_phy->dkl_ssc.mod_val,
			dkl_phy->dkl_dco.addr,
			dkl_phy->dkl_dco.mod_val);
	program_mmio(dkl_phy, 1);
}

/**
* @brief
* This function waits until the DKL programming is
* finished. There is a timer for which time the new values will remain in
* effect. After that timer expires, the original values will be restored.
* @param t - The timer which needs to be deleted
* @return void
*/
void dkl::wait_until_done()
{
	TRACING();
	ddi_sel *ds = get_ds();
	dkl_phy_reg *dkl_phy = (dkl_phy_reg *) ds->phy_data;

	while(!dkl_phy->done && !client_done) {
		usleep(1000);
	}

	// Restore original values in case of app termination
	if (client_done) {
		reset_phy_regs(dkl_phy);
	}

	timer_delete(get_timer());
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
* @return void
*/
void dkl::program_mmio(dkl_phy_reg *pr, int mod)
{
#if !TESTING
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
	if(!READ_OFFSET_DWORD(pr->dkl_pll_div0.addr)) {
		WRITE_OFFSET_DWORD(pr->dkl_index.addr, pr->dkl_index_val);
	}

	WRITE_OFFSET_DWORD(pr->dkl_pll_div0.addr,
			mod ? pr->dkl_pll_div0.mod_val : pr->dkl_pll_div0.orig_val);
	WRITE_OFFSET_DWORD(pr->dkl_visa_serializer.addr,
			mod ? pr->dkl_visa_serializer.mod_val : pr->dkl_visa_serializer.orig_val);
	WRITE_OFFSET_DWORD(pr->dkl_bias.addr,
			mod ? pr->dkl_bias.mod_val : pr->dkl_bias.orig_val);
	WRITE_OFFSET_DWORD(pr->dkl_ssc.addr,
			mod ? pr->dkl_ssc.mod_val : pr->dkl_ssc.orig_val);
	WRITE_OFFSET_DWORD(pr->dkl_dco.addr,
			mod ? pr->dkl_dco.mod_val  : pr->dkl_dco.orig_val);
#endif
}

/*
* @brief
* This function resets the DKL Phy MMIO registers to their
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
void dkl::reset_phy_regs(int sig, siginfo_t *si, void *uc)
{
	user_info *ui = (user_info *) si->si_value.sival_ptr;
	if(!ui) {
		return;
	}

	DBG("timer done\n");
	dkl_phy_reg *dr = (dkl_phy_reg *) ui->get_reg();
	dkl *d = (dkl *) ui->get_type();

	d->reset_phy_regs(dr);
	delete ui;
}

void dkl::reset_phy_regs(dkl_phy_reg *dr)
{
	program_mmio(dr, 0);
	DBG("DEFAULT VALUES\n dkl_pll_div0 [0x%X] =\t 0x%X\n dkl_visa_serializer [0x%X] =\t 0x%X\n "
			"dkl_bias [0x%X] =\t 0x%X\n dkl_ssc [0x%X] =\t 0x%X\n dkl_dco [0x%X] =\t 0x%X\n",
			dr->dkl_pll_div0.addr,
			dr->dkl_pll_div0.orig_val,
			dr->dkl_visa_serializer.addr,
			dr->dkl_visa_serializer.orig_val,
			dr->dkl_bias.addr,
			dr->dkl_bias.orig_val,
			dr->dkl_ssc.addr,
			dr->dkl_ssc.orig_val,
			dr->dkl_dco.addr,
			dr->dkl_dco.orig_val);
	dr->done = 1;
}
