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
/*
 * CNL/ICL Port/COMBO-PHY Registers
 */
#define _ICL_COMBOPHY_A               0x162000
#define _ICL_COMBOPHY_B               0x6C000
#define _EHL_COMBOPHY_C               0x160000
#define _RKL_COMBOPHY_D               0x161000
#define _ADL_COMBOPHY_E               0x16B000
#define _ICL_PORT_COMP                0x100
#define _PICK(__index, ...)           (((const uint32_t []){ __VA_ARGS__ })[__index])
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

extern combo_phy_reg combo_table[];

int find_enabled_combo_phys();
void program_combo_phys(double time_diff, timer_t *t);
void check_if_combo_done();
void program_combo_mmio(combo_phy_reg *pr, int mod);

#endif
