#include <math.h>
#include <debug.h>
#include <signal.h>
#include <unistd.h>
#include <memory.h>
#include "mmio.h"
#include "combo.h"


combo_phy_reg combo_table[] = {
	{REG(DPLL0_CFGCR0), REG(DPLL0_CFGCR1), 0, 1},
	{REG(DPLL1_CFGCR0), REG(DPLL1_CFGCR1), 0, 1},
	{REG(DPLL2_CFGCR0), REG(DPLL2_CFGCR1), 0, 1},
	{REG(DPLL3_CFGCR0), REG(DPLL3_CFGCR1), 0, 1},
};

/*******************************************************************************
 * Description
 *  find_enabled_dplls - This function finds out which dplls are enabled on the
 *  current system. It does this by first finding out the values of
 * 	TRANS_DDI_FUNC_CTL register for all the pipes. Bits 30:27 have the DDI which
 *  this pipe is connected to. Once the DDI is found, we match the DDI with the
 * 	available ones on this platform. Then, we read DPCLKA_CFGCR0 or DPCLKA_CFGCR1
 * 	depending upon which DDI is enabled. Finally, the corresponding clock_bits
 * 	tell us which DPLL is turned on for this pipe. Now, we can go about reading
 * 	the corresponding DPLLs.
 * Parameters
 *	None
 * Return val
 *  void
 ******************************************************************************/
void find_enabled_dplls()
{
	int i, j, val, ddi_select, dpclk;
	reg trans_ddi_func_ctl[] = {
		REG(TRANS_DDI_FUNC_CTL_A),
		REG(TRANS_DDI_FUNC_CTL_B),
		REG(TRANS_DDI_FUNC_CTL_C),
		REG(TRANS_DDI_FUNC_CTL_D),
	};

	/*
	 * According to the BSpec:
	 * 0000b	None
	 * 0001b	DDI A
	 * 0010b	DDI B
	 * 0011b	DDI C
	 * 0100b	DDI USBC1
	 * 0101b	DDI USBC2
	 * 0110b	DDI USBC3
	 * 0111b	DDI USBC4
	 * 1000b	DDI USBC5
	 * 1001b	DDI USBC6
	 * 1000b	DDI D
	 * 1001b	DDI E
	 *
	 * DISPLAY_CCU / DPCLKA Clock	DE Internal Clock
	 * DDIA_DE_CLK	                DDIA
	 * DDIB_DE_CLK	                USBC1
	 * DDII_DE_CLK	                USBC2
	 * DDIJ_DE_CLK	                USBC3
	 * DDIK_DE_CLK	                USBC4
	*/

	for(i = 0; i < ARRAY_SIZE(trans_ddi_func_ctl); i++) {
		/* First read the TRANS_DDI_FUNC_CTL to find if this pipe is enabled or not */
		val = READ_OFFSET_DWORD(trans_ddi_func_ctl[i].addr);
		DBG("0x%X = 0x%X\n", trans_ddi_func_ctl[i].addr, val);
		if(!(val & BIT(31))) {
			DBG("Pipe %d is turned off\n", i+1);
			continue;
		}

		/* TRANS_DDI_FUNC_CTL bits 30:27 have the DDI which this pipe is connected to */
		ddi_select = GETBITS_VAL(val, 30, 27);
		DBG("ddi_select = 0x%X\n", ddi_select);

		for(j = 0; j < platform_table[supported_platform].ds_size; j++) {
			/* Match the DDI with the available ones on this platform */
			if(platform_table[supported_platform].ds[j].de_clk == ddi_select) {

				/* Read DPCLKA_CFGCR0 or DPCLKA_CFGCR1 depending upon which DDI is enabled */
				dpclk = READ_OFFSET_DWORD(platform_table[supported_platform].ds[j].dpclk.addr);
				DBG("0x%X = 0x%X\n", platform_table[supported_platform].ds[j].dpclk.addr, dpclk);
				/* DPCLKA_CFGCR must have the DDI's clock bit set to 0 (turned on) */
				if(dpclk & BIT(platform_table[supported_platform].ds[j].clock_bit)) {
					ERR("Clock is Off\n");
					break;
				}
				/*
				 * Since there are only 2 bits for all DPLLs, it is safe to
				 * assume that the high order bit is just one more than the
				 * low order bit
				 */
				platform_table[supported_platform].ds[j].dpll_num = GETBITS_VAL(dpclk,
						platform_table[supported_platform].ds[j].mux_select_low_bit+1,
						platform_table[supported_platform].ds[j].mux_select_low_bit);
				DBG("DPLL num = 0x%X\n", platform_table[supported_platform].ds[j].dpll_num);
				if(!dpll_enabled_list) {
					dpll_enabled_list = new list<ddi_sel *>;
				}
				ddi_sel *new_node = new ddi_sel;
				memcpy(new_node, &platform_table[supported_platform].ds[j], sizeof(ddi_sel));
				dpll_enabled_list->push_back(new_node);
			}
		}
	}
}

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
	find_enabled_dplls();

	if(dpll_enabled_list) {
		for(list<ddi_sel *>::iterator it = dpll_enabled_list->begin();
			it != dpll_enabled_list->end(); it++) {
			if((*it)->dpll_num >= ARRAY_SIZE(combo_table)) {
				ERR("Invalid dpll_num %d\n", (*it)->dpll_num);
				continue;
			}
			DBG("DPLL %d is enabled with a Combo phy\n", (*it)->dpll_num);
			combo_table[(*it)->dpll_num].enabled = 1;
			enabled++;
		}
	}
#endif
	DBG("Total Combo phys on: %d\n", enabled);
	return enabled;
}

/*******************************************************************************
 * Description
 *	get_val_from_bit- This function finds the value from a bit within a table
 * Parameters
 *	div_val *dt - This is the table to search for a val from.
 *	int dt_size - The size of this table
 *  int bit - The bit to search for
 * Return val
 *	int - The value to return corresponding to the bit
 ******************************************************************************/
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
		int pdiv_bit = GETBITS_VAL(combo_table[i].cfgcr1.orig_val, 5, 2);
		int pdiv = get_val_from_bit(pdiv_table, ARRAY_SIZE(pdiv_table), pdiv_bit);
		int kdiv_bit = GETBITS_VAL(combo_table[i].cfgcr1.orig_val, 8, 6);
		int kdiv = get_val_from_bit(kdiv_table, ARRAY_SIZE(kdiv_table), kdiv_bit);
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

		combo_table[i].cfgcr0.mod_val &= ~GENMASK(24, 0);
		combo_table[i].cfgcr0.mod_val |= i_fbdiv_intgr_9_0;
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

