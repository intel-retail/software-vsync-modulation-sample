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

#ifndef _COMMON_H
#define _COMMON_H

#include <list>

using namespace std;

#define TESTING                       0
#define SHIFT                         (0.1)
#define REF_COMBO_FREQ                19.2
#define ONE_SEC_IN_NS                 (1000 * 1000 * 1000)
#define TV_NSEC(t)                    ((long) ((t * 1000000) % ONE_SEC_IN_NS))
#define TV_SEC(t)                     ((time_t) ((t * 1000000) / ONE_SEC_IN_NS))
#define TIME_IN_USEC(sec, usec)       (unsigned long) (1000000 * sec + usec)
#define BIT(nr)                       (1UL << (nr))
#define ARRAY_SIZE(a)                 (int) (sizeof(a)/sizeof(a[0]))
#define BITS_PER_LONG                 64
#define GENMASK(h, l) \
	(((~0UL) - (1UL << (l)) + 1) & (~0UL >> (BITS_PER_LONG - 1 - (h))))
#define GETBITS_VAL(val, h, l)       ((val & GENMASK(h, l)) >> l)
#define _PICK_EVEN(__index, __a, __b) ((__a) + (__index) * ((__b) - (__a)))
#define _PICK(__index, ...)           (((const uint32_t []){ __VA_ARGS__ })[__index])
#define MAX_DEVICE_ID                 20
/* Per-pipe DDI Function Control */
#define TRANS_DDI_FUNC_CTL_A          0x60400
#define TRANS_DDI_FUNC_CTL_B          0x61400
#define TRANS_DDI_FUNC_CTL_C          0x62400
#define TRANS_DDI_FUNC_CTL_D          0x63400
#define TRANS_DDI_FUNC_CTL_EDP        0x6F400
#define TRANS_DDI_FUNC_CTL_DSI0       0x6b400
#define TRANS_DDI_FUNC_CTL_DSI1       0x6bc00
#define DPCLKA_CFGCR0                 0x164280
#define DPCLKA_CFGCR1                 0x1642BC
#define REG(a)                        {a, 0, 0}

enum {
	DKL,
	COMBO,
//	C10,
	TOTAL_PHYS,
};

typedef struct _reg {
	int addr;
	int orig_val;
	int mod_val;
} reg;

class user_info {
	private:
	int phy_type;
	void *phy_reg;
	public:
	user_info(int t, void *r) { phy_type = t; phy_reg = r; }
	int get_type() { return phy_type; }
	void *get_reg() { return phy_reg; }
};

typedef struct _vbl_info {
	long *vsync_array;
	int size;
	int counter;
	int pipe;
} vbl_info;

typedef int  (*find_func)();
typedef void (*program_func)(double time_diff, timer_t *t);
typedef void (*wait_func)(timer_t t);
typedef void (*reset_func)(int sig, siginfo_t *si, void *uc);

typedef struct _phy_funcs {
	char name[20];
	void *table;
	find_func find;
	program_func program;
	wait_func wait_until_done;
	timer_t timer_id;
} phy_funcs;

typedef struct _ddi_sel {
	char de_clk_name[20];
	int de_clk;
	reg dpclk;
	int clock_bit;
	int mux_select_low_bit;
	int dpll_num;
} ddi_sel;

typedef struct _platform {
	char name[20];
	int device_ids[MAX_DEVICE_ID];
	ddi_sel *ds;
	int ds_size;
} platform;

extern platform platform_table[];
extern int supported_platform;

int calc_steps_to_sync(double time_diff, double shift);
void timer_handler(int sig, siginfo_t *si, void *uc);
int make_timer(long expire_ms, void *user_ptr, timer_t *t, reset_func reset);
unsigned int pipe_to_wait_for(int pipe);

#endif
