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

#ifndef _ADL_S_H
#define _ADL_S_H

#include "../common.h"

ddi_sel adl_s_ddi_sel[] = {
	// name     phy     de_clk  dpclk               clock_bit   mux_select_low_bit  dpll_num    phy_data
	{"DDI_A",   COMBO,  1,      REG(DPCLKA_CFGCR0), 10,         0,                  0,          NULL,},
	{"DDI_C1",  COMBO,  4,      REG(DPCLKA_CFGCR0), 11,         2,                  0,          NULL,},
	{"DDI_C2",  COMBO,  5,      REG(DPCLKA_CFGCR0), 24,         4,                  0,          NULL,},
	{"DDI_C3",  COMBO,  6,      REG(DPCLKA_CFGCR1), 4,          0,                  0,          NULL,},
	{"DDI_C4",  COMBO,  7,      REG(DPCLKA_CFGCR1), 5,          2,                  0,          NULL,},
};


#endif