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

#ifndef _MTL_H
#define _MTL_H

#include "../common.h"

ddi_sel mtl_ddi_sel[] = {
	// name     phy     de_clk  dpclk               clock_bit   mux_select_low_bit  dpll_num/port    phy_data
	{"DDI_A",   C10,  1,      REG(0),             10,         0,                  1,          NULL,},
	{"DDI_B",   C10,  2,      REG(0),             11,         2,                  2,          NULL,},
	{"DDI_TC1", C20,  3,      REG(0),             11,         2,                  3,          NULL,},
	{"DDI_TC2", C20,  4,      REG(0),             11,         2,                  4,          NULL,},
	{"DDI_TC3", C20,  5,      REG(0),             11,         2,                  5,          NULL,},
	{"DDI_TC4", C20,  6,      REG(0),             11,         2,                  6,          NULL,},
};

#endif
