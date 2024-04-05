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

#include "common.h"

#define _ICL_PHY_MISC_A               0x64C00
#define _ICL_PHY_MISC_B               0x64C04
#define _PORT(port, a, b)             _PICK_EVEN(port, a, b)
#define _MMIO_PORT(port, a, b)        _PORT(port, a, b)
#define ICL_PHY_MISC(port)            _MMIO_PORT(port, _ICL_PHY_MISC_A, _ICL_PHY_MISC_B)
#define ICL_PHY_MISC_DE_IO_COMP_PWR_DOWN (1 << 23)
#define COMP_INIT                     (1 << 31)

// CNL/ICL Port/COMBO-PHY Registers
#define _ICL_COMBOPHY_A               0x162000
#define _ICL_COMBOPHY_B               0x6C000
#define _EHL_COMBOPHY_C               0x160000
#define _RKL_COMBOPHY_D               0x161000
#define _ADL_COMBOPHY_E               0x16B000
#define _ICL_PORT_COMP                0x100
#define DPLL0_CFGCR0                  0x164284
#define DPLL0_CFGCR1                  0x164288
#define DPLL1_CFGCR0                  0x16428C
#define DPLL1_CFGCR1                  0x164290
#define DPLL2_CFGCR0                  0x164294
#define DPLL2_CFGCR1                  0x164298
#define DPLL3_CFGCR0                  0x1642C0
#define DPLL3_CFGCR1                  0x1642C4

#define _ICL_COMBOPHY(phy)            _PICK(phy, _ICL_COMBOPHY_A, \
			                               _ICL_COMBOPHY_B, \
				                           _EHL_COMBOPHY_C, \
										   _RKL_COMBOPHY_D, \
										   _ADL_COMBOPHY_E)
#define _ICL_PORT_COMP_DW(dw, phy)    (_ICL_COMBOPHY(phy) + _ICL_PORT_COMP + 4 * (dw))
#define ICL_PORT_COMP_DW0(phy)        _ICL_PORT_COMP_DW(0, phy)
#define READ_VAL(r, v)                combo_table[i].r.v = READ_OFFSET_DWORD(combo_table[i].r.addr);

typedef struct _combo_phy_reg {
	reg cfgcr0;
	reg cfgcr1;
	int enabled;
	int done;
} combo_phy_reg;

typedef struct _div_val {
	int bit;
	int val;
} div_val;

extern combo_phy_reg combo_table[];

int find_enabled_combo_phys();
void program_combo_phys(double time_diff, timer_t *t);
void wait_until_combo_done(timer_t t);
void program_combo_mmio(combo_phy_reg *pr, int mod);
void reset_combo(int sig, siginfo_t *si, void *uc);
void cleanup_list();

#endif
