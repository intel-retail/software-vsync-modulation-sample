#include <math.h>
#include <debug.h>
#include <signal.h>
#include "mmio.h"
#include "combo.h"


combo_phy_reg combo_table[] = {
	{{0x164284, 0, 0}, {0x16428C, 0, 0}, 0, 1},
};

/*******************************************************************************
 * Description
 *	find_enabled_combo_phys - This function finds out which of the Combo phys are
 *	on.
 * Parameters
 *	NONE
 * Return val
 *	int - The number of combo phys that are enabled
 ******************************************************************************/
int find_enabled_combo_phys()
{
	int enabled = 0;
#if TESTING
	enabled++;
	combo_table[0].enabled = 1;
#else
	enabled++;
	combo_table[0].enabled = 1;
	for(int phy = 0; phy < 5; phy++) {
		DBG("misc: 0x%X, val = 0x%X, dw0: 0x%X, enabled: %d\n", ICL_PHY_MISC(phy), ICL_PORT_COMP_DW0(phy),
				READ_OFFSET_DWORD(ICL_PHY_MISC(phy)),
				(!(READ_OFFSET_DWORD(ICL_PHY_MISC(phy)) &
				  ICL_PHY_MISC_DE_IO_COMP_PWR_DOWN) &&
				 (READ_OFFSET_DWORD(ICL_PORT_COMP_DW0(phy)) & COMP_INIT)));
	}
	for(int phy = 0; phy < ARRAY_SIZE(combo_table); phy++) {
		if((READ_OFFSET_DWORD(ICL_PHY_MISC(phy)) &
					ICL_PHY_MISC_DE_IO_COMP_PWR_DOWN) &&
				(READ_OFFSET_DWORD(ICL_PORT_COMP_DW0(phy)) & COMP_INIT)) {
			combo_table[phy].enabled = 1;
			enabled++;
			DBG("Combo phy #%d is on\n", phy);
		}
		combo_table[phy].done = 1;
	}
#endif
	DBG("Total Combo phys on: %d\n", enabled);
	return enabled;
}

/*******************************************************************************
 * Description
 *	program_combo_phys- This function programs Combo phys on the system
 * Parameters
 *	double time_diff - This is the time difference in between the primary and the
 *	secondary systems in ms. If master is ahead of the slave , then the time
 *	difference is a positive number otherwise negative.
 *	timer_t *t     - A pointer to a pointer where we need to store the timer
 * Return val
 *	void
 ******************************************************************************/
void program_combo_phys(double time_diff, timer_t *t)
{
	double shift = SHIFT;

	/* Cycle through all the Combo phys */
	for(int i = 0; i < ARRAY_SIZE(combo_table); i++) {
		/* Skip any that aren't enabled */
		if(!combo_table[i].enabled) {
			continue;
		}
#if TESTING
		/* ADL */
		/*
		combo_table[i].cfgcr0.orig_val = 0x01c001a5;
		combo_table[i].cfgcr1.orig_val = 0x013331cf;
		*/
		/* TGL */
		combo_table[i].cfgcr0.orig_val = 0x00B001B1;
		combo_table[i].cfgcr1.orig_val = 0x00000e84;
#else
		READ_VAL(cfgcr0, orig_val);
		READ_VAL(cfgcr1, orig_val);

		if(time_diff < 0) {
			shift *= -1;
		}

		/*
		 * For whichever PHY we find, let's set the done flag to 0 so that we can later
		 * have a timer for it to reset the default values back in their registers
		 */
		combo_table[i].done = 0;
		int steps = calc_steps_to_sync(time_diff, shift);
		DBG("steps are %d\n", steps);
		user_info *ui = new user_info(COMBO, &combo_table[i]);
		make_timer((long) steps, ui, t);
#endif
		DBG("OLD VALUES\n cfgcr0 \t 0x%X\n cfgcr1 \t 0x%X\n",
				combo_table[i].cfgcr0.orig_val, combo_table[i].cfgcr1.orig_val);

		/*
		 * Symbol clock frequency in MHz (base) = DCO Divider * Reference frequency in MHz /  (5 * Pdiv * Qdiv * Kdiv)
		 * DCO Divider comes from DPLL_CFGCR0 DCO Integer + (DPLL_CFGCR0 DCO Fraction / 2^15)
		 * Pdiv from DPLL_CFGCR1 Pdiv
		 * Qdiv from DPLL_CFGCR1 Qdiv Mode ? DPLL_CFGCR1 Qdiv Ratio : 1
		 * Kdiv from DPLL_CFGCR1 Kdiv
		 */
		int i_fbdiv_intgr_9_0 = GETBITS_VAL(combo_table[i].cfgcr0.orig_val, 9, 0);
		int i_fbdivfrac_14_0 = GETBITS_VAL(combo_table[i].cfgcr0.orig_val, 24, 10);
		int pdiv = GETBITS_VAL(combo_table[i].cfgcr1.orig_val, 5, 2) + 1;
		int kdiv = GETBITS_VAL(combo_table[i].cfgcr1.orig_val, 8, 6);
		/* TODO: In case we run into some other weird dividers, then we may need to revisit this */
		int qdiv = (kdiv == 2) ? GETBITS_VAL(combo_table[i].cfgcr1.orig_val, 17, 10) : 1;
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
		combo_table[i].cfgcr0.mod_val &= ~GENMASK(9, 0);
		combo_table[i].cfgcr0.mod_val |= i_fbdiv_intgr_9_0;
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

		combo_table[i].cfgcr0.mod_val &= ~GENMASK(24, 10);
		combo_table[i].cfgcr0.mod_val |= (long) new_i_fbdivfrac_14_0 << 10;

		DBG("NEW VALUES\n cfgcr0 \t 0x%X\n", combo_table[i].cfgcr0.mod_val);
		program_combo_mmio(&combo_table[i], 1);
	}
}

/*******************************************************************************
 * Description
 *	check_if_combo_done - This function checks to see if the Combo programming is
 *	finished. There is a timer for which time the new values will remain in
 *	effect. After that timer expires, the original values will be restored.
 * Parameters
 *	NONE
 * Return val
 *	void
 ******************************************************************************/
void check_if_combo_done()
{
	TRACING();
	/* Wait to write back the original value */
	for(int i = 0; i < ARRAY_SIZE(combo_table); i++) {
		while(!combo_table[i].done) {
			usleep(1000);
		}
	}
}

/*******************************************************************************
 * Description
 *  program_combo_mmio - This function programs the Combo Phy MMIO registers
 *  needed to move avsync period for a system.
 * Parameters
 *	combo_phy_reg *pr - The data structure that holds all of the PHY registers
 *	that need to be programmed
 *	int mod - This parameter tells the function whether to program the original
 *	values or the modified ones. 0 = Original, 1 = modified
 * Return val
 *  void
 ******************************************************************************/
void program_combo_mmio(combo_phy_reg *pr, int mod)
{
#if !TESTING
	WRITE_OFFSET_DWORD(pr->cfgcr0.addr,
			mod ? pr->cfgcr0.mod_val : pr->cfgcr0.orig_val);
#endif
}

