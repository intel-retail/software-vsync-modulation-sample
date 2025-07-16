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

#ifndef _DP_M_N_H
#define _DP_M_N_H

#include "phy.h"

typedef struct _dp_m_n_phy_reg {
	reg mreg;
	reg nreg;
	int enabled;
} dp_m_n_phy_reg;

class dp_m_n : public phys {
public:
	dp_m_n(ddi_sel* ds, int _pipe);
	~dp_m_n( );

	int program_mmio(int mod);
	double calculate_pll_clock( );
	int calculate_feedback_dividers(double pll_freq);
	void print_registers( );
	void read_registers( );

private:
	dp_m_n_phy_reg* dp_m_n_phy;
};

extern dp_m_n_phy_reg dp_m_n_table[];


#endif
