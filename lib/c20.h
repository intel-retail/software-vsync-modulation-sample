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

#ifndef _C20_H
#define _C20_H

#include "phy.h"
#include "cx0_helper.h"

using namespace cx0;

#define RAWCMN_DIG_MPLLA_FRAC_UPDATE 0x110  // Atomic mode Bit 1, Update Bit 0
#define RAWCMN_DIG_MPLLA_CNTX_CFG_0  0x128  // Multiplier, 0-11,
#define RAWCMN_DIG_MPLLA_CNTX_CFG_1  0x129
#define RAWCMN_DIG_MPLLA_CNTX_CFG_2  0x12a
#define RAWCMN_DIG_MPLLA_CNTX_CFG_3  0x12b
#define RAWCMN_DIG_MPLLA_CNTX_CFG_4  0x12c
#define RAWCMN_DIG_MPLLA_CNTX_CFG_5  0x12d
#define RAWCMN_DIG_MPLLA_CNTX_CFG_6  0x12e  // Fraction EN, Bit 13
#define RAWCMN_DIG_MPLLA_CNTX_CFG_7  0x12f  // Frac Deno 0 - 15
#define RAWCMN_DIG_MPLLA_CNTX_CFG_8  0x130  // Frac Quot 0 - 15
#define RAWCMN_DIG_MPLLA_CNTX_CFG_9  0x131  // Frac Rem 0 - 15


#define RAWCMN_DIG_MPLLB_FRAC_UPDATE 0x111  // Atomic mode Bit 1, Update Bit 0
#define RAWCMN_DIG_MPLLB_CNTX_CFG_0  0x134  // Multiplier, 0-11,
#define RAWCMN_DIG_MPLLB_CNTX_CFG_1  0x135
#define RAWCMN_DIG_MPLLB_CNTX_CFG_2  0x136
#define RAWCMN_DIG_MPLLB_CNTX_CFG_3  0x137
#define RAWCMN_DIG_MPLLB_CNTX_CFG_4  0x138
#define RAWCMN_DIG_MPLLB_CNTX_CFG_5  0x139
#define RAWCMN_DIG_MPLLB_CNTX_CFG_6  0x13a  // Fraction EN, Bit 13
#define RAWCMN_DIG_MPLLB_CNTX_CFG_7  0x13b  // Frac Deno 0 - 15
#define RAWCMN_DIG_MPLLB_CNTX_CFG_8  0x13c  // Frac Quot 0 - 15
#define RAWCMN_DIG_MPLLB_CNTX_CFG_9  0x13d  // Frac Rem 0 - 15

#define RAWCMN_DIG_CFG_INDEX_0  0  // Multiplier, 0-11,
#define RAWCMN_DIG_CFG_INDEX_1  1
#define RAWCMN_DIG_CFG_INDEX_2  2
#define RAWCMN_DIG_CFG_INDEX_3  3
#define RAWCMN_DIG_CFG_INDEX_4  4
#define RAWCMN_DIG_CFG_INDEX_5  5
#define RAWCMN_DIG_CFG_INDEX_6  6  // Fraction EN, Bit 13,
#define RAWCMN_DIG_CFG_INDEX_7  7  // Frac Deno 0 - 15,
#define RAWCMN_DIG_CFG_INDEX_8  8  // Frac Quot 0 - 15,
#define RAWCMN_DIG_CFG_INDEX_9  9  // Frac Rem 0 - 15,

// From Xe/i915 KMD driver code
#define C20_TX_RATE_MULT 1
#define C20_CLK_DIV4_EN 0

#define C20_PLL_REG_COUNT 10

typedef struct _c20_phy_reg {
	u16 tx[3];
	u16 cmn[4];
	u16 frac_en, frac_quot, frac_rem, frac_den, tx_rate, frac_quot_mod, frac_rem_mod;
	u16 multiplier, tx_rate_mult, tx_clk_div, ref_clk_mpllb_div, fb_clk_div4_en;
	u16 pll_state_orig[C20_PLL_REG_COUNT];
	u16 pll_state_mod[C20_PLL_REG_COUNT];
} c20_phy_reg;

class c20 : public phys {
public:
	c20(ddi_sel* ds, int _pipe);
	~c20() {};

	int program_mmio(int mod);
	double calculate_pll_clock();
	int calculate_feedback_dividers(double pll_freq);
	void print_registers();
	void read_registers();

private:
	c20_phy_reg c20_reg;

};

extern c20_phy_reg c20_table[];

#endif
