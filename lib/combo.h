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

#ifndef _COMBO_H
#define _COMBO_H

#include "phy.h"

#define _PORT(port, a, b)             _PICK_EVEN(port, a, b)
#define _MMIO_PORT(port, a, b)        _PORT(port, a, b)
#define ICL_PHY_MISC_DE_IO_COMP_PWR_DOWN (1 << 23)
#define COMP_INIT                     (1 << 31)

 // CNL/ICL Port/COMBO-PHY Registers
#define DPLL0_CFGCR0                  0x164284
#define DPLL0_CFGCR1                  0x164288
#define DPLL1_CFGCR0                  0x16428C
#define DPLL1_CFGCR1                  0x164290
#define DPLL2_CFGCR0                  0x164294
#define DPLL2_CFGCR1                  0x164298
#define DPLL3_CFGCR0                  0x1642C0
#define DPLL3_CFGCR1                  0x1642C4

#define READ_VAL(r, v)                combo_phy->r.v = READ_OFFSET_DWORD(combo_phy->r.addr);

typedef struct _combo_phy_reg {
	reg cfgcr0;
	reg cfgcr1;
	int enabled;
} combo_phy_reg;

typedef struct _div_val {
	int bit;
	int val;
} div_val;

class combo : public phys {
public:
	combo(ddi_sel* ds, int _pipe);
	~combo( );

	int program_mmio(int mod);
	double calculate_pll_clock(uint32_t cfgcr0, uint32_t cfgcr1);
	double calculate_pll_clock( );
	int calculate_feedback_dividers(double pll_freq);
	void calculate_feedback_dividers(double pll_freq, uint32_t cfgcr1,
		uint32_t* i_fbdiv_intgr_9_0, uint32_t* i_fbdivfrac_14_0);
	void print_registers( );
	void read_registers( );

private:
	combo_phy_reg* combo_phy;
	int dpll_num;
};

extern combo_phy_reg combo_table[];

#endif
