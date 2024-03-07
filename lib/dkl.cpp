#include <math.h>
#include <debug.h>
#include <signal.h>
#include <unistd.h>
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

/*******************************************************************************
 * Description
 *	find_enabled_dkl_phys - This function finds out which of the DKL phys are on
 *	It does this by querying the DKL_PLL_DIV0 register for all possible
 *	combinations
 * Parameters
 *	NONE
 * Return val
 *	int - The number of DKL phys that are enabled
 ******************************************************************************/
int find_enabled_dkl_phys()
{
	int enabled = 0;
#if TESTING
	enabled++;
	dkl_table[0].enabled = 1;
#else
	unsigned int val;
	for(int i = 0; i < ARRAY_SIZE(dkl_table); i++) {
		val = READ_OFFSET_DWORD(dkl_table[i].dkl_pll_div0.addr);
		if(val != 0xFFFFFFFF) {
			dkl_table[i].enabled = 1;
			enabled++;
			DBG("DKL phy #%d is on\n", i);
		}
		dkl_table[i].done = 1;
	}
#endif
	DBG("Total DKL phys on: %d\n", enabled);
	return enabled;
}

/*******************************************************************************
 * Description
 *	program_dkl_phys - This function programs DKL phys on the system
 * Parameters
 *	double time_diff - This is the time difference in between the primary and the
 *	secondary systems in ms. If master is ahead of the slave , then the time
 *	difference is a positive number otherwise negative.
 *	timer_t *t     - A pointer to a pointer where we need to store the timer
 * Return val
 *	void
 ******************************************************************************/
void program_dkl_phys(double time_diff, timer_t *t)
{
	double shift = SHIFT;

	for(int i = 0; i < ARRAY_SIZE(dkl_table); i++) {
		/* Skip any that aren't enabled */
		if(!dkl_table[i].enabled) {
			continue;
		}
#if TESTING
		dkl_table[i].dkl_pll_div0.orig_val = 0x50284274;
		dkl_table[i].dkl_visa_serializer.orig_val = 0x54321000;
		dkl_table[i].dkl_bias.orig_val = 0XC1000000;
		dkl_table[i].dkl_ssc.orig_val = 0x400020ff;
		dkl_table[i].dkl_dco.orig_val = 0xe4004080;
#else
		dkl_table[i].dkl_pll_div0.orig_val         = dkl_table[i].dkl_pll_div0.mod_val        = READ_OFFSET_DWORD(dkl_table[i].dkl_pll_div0.addr);

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
		if(!dkl_table[i].dkl_pll_div0.orig_val) {
			WRITE_OFFSET_DWORD(dkl_table[i].dkl_index.addr, dkl_table[i].dkl_index_val);
			dkl_table[i].dkl_pll_div0.orig_val         = dkl_table[i].dkl_pll_div0.mod_val        = READ_OFFSET_DWORD(dkl_table[i].dkl_pll_div0.addr);
		}

		dkl_table[i].dkl_visa_serializer.orig_val  = dkl_table[i].dkl_visa_serializer.mod_val = READ_OFFSET_DWORD(dkl_table[i].dkl_visa_serializer.addr);
		dkl_table[i].dkl_bias.orig_val             = dkl_table[i].dkl_bias.mod_val            = READ_OFFSET_DWORD(dkl_table[i].dkl_bias.addr);
		dkl_table[i].dkl_ssc.orig_val              = dkl_table[i].dkl_ssc.mod_val             = READ_OFFSET_DWORD(dkl_table[i].dkl_ssc.addr);
		dkl_table[i].dkl_dco.orig_val              = dkl_table[i].dkl_dco.mod_val             = READ_OFFSET_DWORD(dkl_table[i].dkl_dco.addr);
		/*
		 * For whichever PHY we find, let's set the done flag to 0 so that we can later
		 * have a timer for it to reset the default values back in their registers
		 */
		dkl_table[i].done = 0;

		if(time_diff < 0) {
			shift *= -1;
		}
		int steps = calc_steps_to_sync(time_diff, shift);
		DBG("steps are %d\n", steps);
		user_info *ui = new user_info(DKL, &dkl_table[i]);
		make_timer((long) steps, ui, t, reset_dkl);
#endif
		DBG("OLD VALUES\n dkl_pll_div0 \t 0x%X\n dkl_visa_serializer \t 0x%X\n "
				"dkl_bias \t 0x%X\n dkl_ssc \t 0x%X\n dkl_dco \t 0x%X\n",
				dkl_table[i].dkl_pll_div0.orig_val,
				dkl_table[i].dkl_visa_serializer.orig_val,
				dkl_table[i].dkl_bias.orig_val,
				dkl_table[i].dkl_ssc.orig_val,
				dkl_table[i].dkl_dco.orig_val);

		/*
		 * PLL frequency in MHz (base) = 38.4 * DKL_PLL_DIV0[i_fbprediv_3_0] *
		 *		( DKL_PLL_DIV0[i_fbdiv_intgr_7_0]  + DKL_BIAS[i_fbdivfrac_21_0] / 2^22 )
		 */
		int i_fbprediv_3_0    = GETBITS_VAL(dkl_table[i].dkl_pll_div0.orig_val, 11, 8);
		int i_fbdiv_intgr_7_0 = GETBITS_VAL(dkl_table[i].dkl_pll_div0.orig_val, 7, 0);
		int i_fbdivfrac_21_0  = GETBITS_VAL(dkl_table[i].dkl_bias.orig_val, 29, 8);
		double pll_freq = (double) (REF_DKL_FREQ * i_fbprediv_3_0 * (i_fbdiv_intgr_7_0 + i_fbdivfrac_21_0 / pow(2, 22)));
		double new_pll_freq = pll_freq + (shift * pll_freq / 100);
		double new_i_fbdivfrac_21_0 = ((new_pll_freq / (REF_DKL_FREQ * i_fbprediv_3_0)) - i_fbdiv_intgr_7_0) * pow(2,22);

		if(new_i_fbdivfrac_21_0 < 0) {
			i_fbdiv_intgr_7_0 -= 1;
			dkl_table[i].dkl_pll_div0.mod_val &= ~GENMASK(7, 0);
			dkl_table[i].dkl_pll_div0.mod_val |= i_fbdiv_intgr_7_0;
			new_i_fbdivfrac_21_0 = ((new_pll_freq / (REF_DKL_FREQ * i_fbprediv_3_0)) - (i_fbdiv_intgr_7_0)) * pow(2,22);
		}

		DBG("old pll_freq \t %f\n", pll_freq);
		DBG("new_pll_freq \t %f\n", new_pll_freq);
		DBG("new fbdivfrac \t 0x%X\n", (int) new_i_fbdivfrac_21_0);

		dkl_table[i].dkl_bias.mod_val &= ~GENMASK(29, 8);
		dkl_table[i].dkl_bias.mod_val |= (long)new_i_fbdivfrac_21_0 << 8;
		dkl_table[i].dkl_visa_serializer.mod_val &= ~GENMASK(2, 0);
		dkl_table[i].dkl_visa_serializer.mod_val |= 0x200;
		dkl_table[i].dkl_ssc.mod_val &= ~GENMASK(31, 29);
		dkl_table[i].dkl_ssc.mod_val |= 0x2 << 29;
		dkl_table[i].dkl_ssc.mod_val &= ~BIT(13);
		dkl_table[i].dkl_ssc.mod_val |= 0x2 << 12;
		dkl_table[i].dkl_dco.mod_val &= ~BIT(2);
		dkl_table[i].dkl_dco.mod_val |= 0x2 << 1;

		DBG("NEW VALUES\n dkl_pll_div0 \t 0x%X\n dkl_visa_serializer \t 0x%X\n "
				"dkl_bias \t 0x%X\n dkl_ssc \t 0x%X\n dkl_dco \t 0x%X\n",
				dkl_table[i].dkl_pll_div0.mod_val,
				dkl_table[i].dkl_visa_serializer.mod_val,
				dkl_table[i].dkl_bias.mod_val,
				dkl_table[i].dkl_ssc.mod_val,
				dkl_table[i].dkl_dco.mod_val);
		program_dkl_mmio(&dkl_table[i], 1);
	}
}

/*******************************************************************************
 * Description
 *	check_if_dkl_done - This function checks to see if the DKL programming is
 *	finished. There is a timer for which time the new values will remain in
 *	effect. After that timer expires, the original values will be restored.
 * Parameters
 *	NONE
 * Return val
 *	void
 ******************************************************************************/
void check_if_dkl_done()
{
	TRACING();
	for(int i = 0; i < ARRAY_SIZE(dkl_table); i++) {
		while(!dkl_table[i].done) {
			usleep(1000);
		}
	}
}

/*******************************************************************************
 * Description
 *  program_dkl_mmio - This function programs the DKL Phy MMIO registers needed
 *  to move a vsync period for a system.
 * Parameters
 *	dkl_phy_reg *pr - The data structure that holds all of the PHY registers that
 *	need to be programmed
 *	int mod - This parameter tells the function whether to program the original
 *	values or the modified ones. 0 = Original, 1 = modified
 * Return val
 *  void
 ******************************************************************************/
void program_dkl_mmio(dkl_phy_reg *pr, int mod)
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

/*******************************************************************************
 * Description
 *  reset_dkl - This function resets the DKL Phy MMIO registers to their
 *  original value. It gets executed whenever a timer expires. We program MMIO
 *	registers of the PHY in this function becase we have waited for a certain
 *  time period to get the primary and secondary systems vsync in sync and now
 *	it is time to reprogram the default values for the secondary system's PHYs.
 * Parameters
 *	int sig - The signal that fired
 *	siginfo_t *si - A pointer to a siginfo_t, which is a structure containing
 *  further information about the signal
 *	void *uc - This is a pointer to a ucontext_t structure, cast to void *.
 *  The structure pointed to by this field contains signal context information
 *  that was saved on the user-space stack by the kernel
 * Return val
 *  void
 ******************************************************************************/
void reset_dkl(int sig, siginfo_t *si, void *uc)
{
	user_info *ui = (user_info *) si->si_value.sival_ptr;
	if(!ui) {
		return;
	}

	DBG("timer done\n");
	dkl_phy_reg *dr = (dkl_phy_reg *) ui->get_reg();
	program_dkl_mmio(dr, 0);
	DBG("DEFAULT VALUES\n dkl_pll_div0 \t 0x%X\n dkl_visa_serializer \t 0x%X\n "
			"dkl_bias \t 0x%X\n dkl_ssc \t 0x%X\n dkl_dco \t 0x%X\n",
			dr->dkl_pll_div0.orig_val,
			dr->dkl_visa_serializer.orig_val,
			dr->dkl_bias.orig_val,
			dr->dkl_ssc.orig_val,
			dr->dkl_dco.orig_val);
	dr->done = 1;
	delete ui;
}
