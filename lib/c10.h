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

#ifndef _C10_H
#define _C10_H

#include "phy.h"
#include "cx0_helper.h"

using namespace cx0;

typedef struct _c10_phy_reg {
	u8 pll_state_orig[C10_PLL_REG_COUNT];
	u8 pll_state_mod[C10_PLL_REG_COUNT];
} c10_phy_reg;

class c10 : public phys {
public:
	c10(ddi_sel* ds, int _pipe);
	~c10() {};

	int program_mmio(int mod);
	double calculate_pll_clock();
	int calculate_feedback_dividers(double pll_freq);
	void print_registers();
	void read_registers();

private:
	c10_phy_reg c10_reg;

};

extern c10_phy_reg c10_table[];


#endif
