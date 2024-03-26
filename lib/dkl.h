// Copyright (C) 2023 Intel Corporation
// SPDX-License-Identifier: MIT

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

extern dkl_phy_reg dkl_table[];

int find_enabled_dkl_phys();
void program_dkl_phys(double time_diff, timer_t *t);
void check_if_dkl_done();
void program_dkl_mmio(dkl_phy_reg *pr, int mod);
void reset_dkl(int sig, siginfo_t *si, void *uc);

#endif
