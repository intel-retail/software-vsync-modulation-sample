#ifndef _COMMON_H
#define _COMMON_H

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

enum {
	DKL,
	COMBO,
	C10,
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

int calc_steps_to_sync(double time_diff, double shift);
void timer_handler(int sig, siginfo_t *si, void *uc);
int make_timer(long expire_ms, void *user_ptr, timer_t *t);

#endif
