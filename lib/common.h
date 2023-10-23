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
#define TRANS_VBLANK_A                0x60010
#define TRANS_VTOTAL_A                0x6000c
#define TRANS_VTOTAL_B                0x6100c
#define TRANS_VBLANK_B                0x61010
#define TRANS_VTOTAL_C                0x6200c
#define TRANS_VBLANK_C                0x62010
#define TRANS_VTOTAL_D                0x6300c
#define TRANS_VBLANK_D                0x63010
#define TRANS_VTOTAL_EDP              0x6f00c
#define TRANS_VBLANK_EDP              0x6f014
#define REG(a)                        {a, 0, 0}

enum {
	PIPE,
	TOTAL_PHYS,
};

typedef struct _reg {
	int addr;
	int orig_val;
	int mod_val;
} reg;

class user_info {
	private:
	void *phy_reg;
	public:
	user_info(void *r) { phy_reg = r; }
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
typedef void (*check_func)();

typedef struct _phy_funcs {
	char name[20];
	void *table;
	find_func find;
	program_func program;
	check_func check_if_done;
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

typedef struct _trans_reg {
	reg vblank;
	reg vtotal;
	int enabled;
	int done;
	timer_t timer_id;
} trans_reg;

int calc_time_to_sync(int vblank, double time_diff);
void timer_handler(int sig, siginfo_t *si, void *uc);
int make_timer(long expire_ms, void *user_ptr, timer_t *t);
void program_regs(trans_reg *tr, int mod);
unsigned int pipe_to_wait_for(int pipe);

#endif
