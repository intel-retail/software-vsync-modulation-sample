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

#ifndef _DKL_H
#define _DKL_H

#include "common.h"

#define REF_DKL_FREQ                  38.4
#define PHY_BASE                      0x168000
#define PHY_NUM_BASE(phy_num)         (PHY_BASE + phy_num * 0x1000)
#define DKL_PLL_DIV0(phy_num)         (PHY_NUM_BASE(phy_num) + 0x200)
#define DKL_SSC(phy_num)              (PHY_NUM_BASE(phy_num) + 0x210)
#define DKL_BIAS(phy_num)             (PHY_NUM_BASE(phy_num) + 0x214)
#define DKL_VISA_SERIALIZER(phy_num)  (PHY_NUM_BASE(phy_num) + 0x220)
#define DKL_DCO(phy_num)              (PHY_NUM_BASE(phy_num) + 0x224)
#define _HIP_INDEX_REG0               0x1010A0
#define _HIP_INDEX_REG1               0x1010A4
#define HIP_INDEX_REG(tc_port)        ((tc_port) < 4 ? _HIP_INDEX_REG0 : _HIP_INDEX_REG1)
#define _HIP_INDEX_SHIFT(tc_port)     (8 * ((tc_port) % 4))
#define HIP_INDEX_VAL(tc_port, val)   ((val) << _HIP_INDEX_SHIFT(tc_port))

typedef struct _dkl_phy_reg {
	reg dkl_pll_div0;
	reg dkl_visa_serializer;
	reg dkl_bias;
	reg dkl_ssc;
	reg dkl_dco;
	reg dkl_index;
	int dkl_index_val;
	int enabled;
	int done;
} dkl_phy_reg;

class dkl : public phys {
public:
	dkl(ddi_sel *ds, int first_dkl_phy_loc);
	~dkl() {};

	void program_mmio(dkl_phy_reg *pr, int mod);
	static void reset_phy_regs(int sig, siginfo_t *si, void *uc);

	void program_phy(double time_diff);
	void wait_until_done();
};

extern dkl_phy_reg dkl_table[];

#endif
