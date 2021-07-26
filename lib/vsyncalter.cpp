#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <vsyncalter.h>
#include <debug.h>
#include <stdlib.h>
#include <memory.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include "mmio.h"
#include <xf86drm.h>

#define TESTING                       0
#define SHIFT                         (0.1)
#define REF_FREQ                      38.4
#define ONE_VSYNC_PERIOD              16.666
#define ONE_SEC_IN_NS                 (1000 * 1000 * 1000)
#define TV_NSEC(t)                    ((long) ((t * 1000000) % ONE_SEC_IN_NS))
#define TV_SEC(t)                     ((time_t) ((t * 1000000) / ONE_SEC_IN_NS))
#define TIME_IN_USEC(t)               (unsigned long) (1000000 * t.tv_sec + t.tv_usec)
#define BIT(nr)                       (1UL << (nr))
#define BITS_PER_LONG                 64
#define GENMASK(h, l) \
	(((~0UL) - (1UL << (l)) + 1) & (~0UL >> (BITS_PER_LONG - 1 - (h))))
#define PHY_BASE                      0x168000
#define PHY_NUM_BASE(phy_num)         (PHY_BASE + phy_num * 0x1000)
#define DKL_PLL_DIV0(phy_num)         (PHY_NUM_BASE(phy_num) + 0x200)
#define DKL_SSC(phy_num)              (PHY_NUM_BASE(phy_num) + 0x210)
#define DKL_BIAS(phy_num)             (PHY_NUM_BASE(phy_num) + 0x214)
#define DKL_VISA_SERIALIZER(phy_num)  (PHY_NUM_BASE(phy_num) + 0x220)
#define DKL_DCO(phy_num)              (PHY_NUM_BASE(phy_num) + 0x224)

typedef struct _phy_regs {
	int dkl_pll_div0;
	int dkl_visa_serializer;
	int dkl_bias;
	int dkl_ssc;
	int dkl_dco;
} phy_regs;

typedef struct _vbl_info {
	long *vsync_array;
	int size;
	int counter;
} vbl_info;


int g_def_phy_num = 0;
int g_done = 0;
int g_dev_fd = 0;
phy_regs g_orig_phy_regs, mod;

int open_device()
{
	return open("/dev/dri/card0", O_RDWR | O_CLOEXEC, 0);
}

void close_device()
{
	close(g_dev_fd);
}

int vsync_lib_init()
{
	if(!IS_INIT()) {
		g_dev_fd = open_device();
		if(g_dev_fd < 0) {
			return 1;
		}
		if(!map_mmio()) {
			close_device();
			return 1;
		}
		g_init = 1;
		g_def_phy_num = 3;
	}

	return 0;
}

void vsync_lib_uninit()
{
	close_device();
	close_mmio_handle();
	g_init = 0;
}

static inline int calc_steps_to_sync(double time_diff, double shift)
{
	return (int) ((time_diff * 100) / (shift * ONE_VSYNC_PERIOD));
}

void program_mmio(phy_regs *pr)
{
#if !TESTING
	WRITE_OFFSET_DWORD(g_mmio, DKL_PLL_DIV0(g_def_phy_num), pr->dkl_pll_div0);
	WRITE_OFFSET_DWORD(g_mmio, DKL_VISA_SERIALIZER(g_def_phy_num), pr->dkl_visa_serializer);
	WRITE_OFFSET_DWORD(g_mmio, DKL_BIAS(g_def_phy_num), pr->dkl_bias);
	WRITE_OFFSET_DWORD(g_mmio, DKL_SSC(g_def_phy_num), pr->dkl_ssc);
	WRITE_OFFSET_DWORD(g_mmio, DKL_DCO(g_def_phy_num), pr->dkl_dco);
#endif
}

static void timer_handler(int sig, siginfo_t *si, void *uc)
{
	PRINT("timer done\n");
	program_mmio(&g_orig_phy_regs);
	DBG("DEFAULT VALUES\n dkl_pll_div0 = 0x%X\n dkl_visa_serializer = 0x%X\n "
			"dkl_bias = 0x%X\n dkl_ssc = 0x%X\n dkl_dco = 0x%X\n",
			g_orig_phy_regs.dkl_pll_div0, g_orig_phy_regs.dkl_visa_serializer,
			g_orig_phy_regs.dkl_bias, g_orig_phy_regs.dkl_ssc, g_orig_phy_regs.dkl_dco);
	g_done = 1;
}

static int make_timer(long expire_ms)
{
	struct sigevent         te;
	struct itimerspec       its;
	struct sigaction        sa;
	int                     sig_no = SIGRTMIN;
	timer_t                 timer_id;

	/* Set up signal handler. */
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = timer_handler;
	sigemptyset(&sa.sa_mask);
	if (sigaction(sig_no, &sa, NULL) == -1) {
		ERR("Failed to setup signal handling for timer.\n");
		return -1;
	}

	/* Set and enable alarm */
	te.sigev_notify = SIGEV_SIGNAL;
	te.sigev_signo = sig_no;
	te.sigev_value.sival_ptr = &timer_id;
	timer_create(CLOCK_REALTIME, &te, &timer_id);

	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = TV_NSEC(expire_ms);
	its.it_value.tv_sec = TV_SEC(expire_ms);
	its.it_value.tv_nsec = TV_NSEC(expire_ms);
	timer_settime(timer_id, 0, &its, NULL);

	return(0);
}

/*
 * time_diff - This is the time difference in between the master and the slave
 * systems. If master is ahead of the slave , then the time difference is a
 * positive number otherwise negative.
 */
void synchronize_vsync(double time_diff)
{
	int  steps;
	double shift = SHIFT;

	if(!IS_INIT()) {
		ERR("Uninitialized lib, please call lib init first\n");
		return;
	}

	memset(&g_orig_phy_regs, 0, sizeof(phy_regs));
	memset(&mod, 0, sizeof(phy_regs));

#if TESTING
	g_orig_phy_regs.dkl_pll_div0 = 0x50284274;
	g_orig_phy_regs.dkl_visa_serializer = 0x54321000;
	g_orig_phy_regs.dkl_bias = 0XC1000000;
	g_orig_phy_regs.dkl_ssc = 0x400020ff;
	g_orig_phy_regs.dkl_dco = 0xe4004080;
#else
	g_orig_phy_regs.dkl_pll_div0 = READ_OFFSET_DWORD(g_mmio, DKL_PLL_DIV0(g_def_phy_num));
	g_orig_phy_regs.dkl_visa_serializer = READ_OFFSET_DWORD(g_mmio, DKL_VISA_SERIALIZER(g_def_phy_num));
	g_orig_phy_regs.dkl_bias = READ_OFFSET_DWORD(g_mmio, DKL_BIAS(g_def_phy_num));
	g_orig_phy_regs.dkl_ssc = READ_OFFSET_DWORD(g_mmio, DKL_SSC(g_def_phy_num));
	g_orig_phy_regs.dkl_dco = READ_OFFSET_DWORD(g_mmio, DKL_DCO(g_def_phy_num));
#endif
	DBG("OLD VALUES\n dkl_pll_div0 = 0x%X\n dkl_visa_serializer = 0x%X\n "
			"dkl_bias = 0x%X\n dkl_ssc = 0x%X\n dkl_dco = 0x%X\n",
			g_orig_phy_regs.dkl_pll_div0, g_orig_phy_regs.dkl_visa_serializer,
			g_orig_phy_regs.dkl_bias, g_orig_phy_regs.dkl_ssc, g_orig_phy_regs.dkl_dco);

	memcpy(&mod, &g_orig_phy_regs, sizeof(phy_regs));

	if(time_diff < 0) {
		shift *= -1;
	}
	steps = calc_steps_to_sync(time_diff, shift);
	PRINT("steps are %d\n", steps);
	make_timer((long) steps * ONE_VSYNC_PERIOD);

	/*
	 * PLL frequency in MHz (base) = 38.4 * DKL_PLL_DIV0[i_fbprediv_3_0] *
	 *		( DKL_PLL_DIV0[i_fbdiv_intgr_7_0]  + DKL_BIAS[i_fbdivfrac_21_0] / 2^22 )
	 */
	int i_fbprediv_3_0    = (g_orig_phy_regs.dkl_pll_div0 & GENMASK(11, 8)) >> 8;
	int i_fbdiv_intgr_7_0 = g_orig_phy_regs.dkl_pll_div0 & GENMASK(7, 0);
	int i_fbdivfrac_21_0  = (g_orig_phy_regs.dkl_bias & GENMASK(29, 8)) >> 8;
	double pll_freq = (double) (REF_FREQ * i_fbprediv_3_0 * (i_fbdiv_intgr_7_0 + i_fbdivfrac_21_0 / pow(2, 22)));
	double new_pll_freq = pll_freq + (shift * pll_freq / 100);
	double new_i_fbdivfrac_21_0 = ((new_pll_freq / (REF_FREQ * i_fbprediv_3_0)) - i_fbdiv_intgr_7_0) * pow(2,22);

	if(new_i_fbdivfrac_21_0 < 0) {
		i_fbdiv_intgr_7_0 -= 1;
		mod.dkl_pll_div0 &= ~GENMASK(7, 0);
		mod.dkl_pll_div0 |= i_fbdiv_intgr_7_0;
		new_i_fbdivfrac_21_0 = ((new_pll_freq / (REF_FREQ * i_fbprediv_3_0)) - (i_fbdiv_intgr_7_0)) * pow(2,22);
	}

	DBG("old pll_freq = %f, new_pll_freq = %f\n", pll_freq, new_pll_freq);
	DBG("new fbdivfrac = %d = 0x%X\n", (int) new_i_fbdivfrac_21_0, (int) new_i_fbdivfrac_21_0);

	mod.dkl_bias &= ~GENMASK(29, 8);
	mod.dkl_bias |= (long)new_i_fbdivfrac_21_0 << 8;
	mod.dkl_visa_serializer &= ~GENMASK(2, 0);
	mod.dkl_visa_serializer |= 0x200;
	mod.dkl_ssc &= ~GENMASK(31, 29);
	mod.dkl_ssc |= 0x2 << 29;
	mod.dkl_ssc &= ~BIT(13);
	mod.dkl_ssc |= 0x2 << 12;
	mod.dkl_dco &= ~BIT(2);
	mod.dkl_dco |= 0x2 << 1;

	DBG("NEW VALUES\n dkl_pll_div0 = 0x%X\n dkl_visa_serializer = 0x%X\n "
			"dkl_bias = 0x%X\n dkl_ssc = 0x%X\n dkl_dco = 0x%X\n",
			mod.dkl_pll_div0, mod.dkl_visa_serializer, mod.dkl_bias, mod.dkl_ssc, mod.dkl_dco);
	program_mmio(&mod);

	/* Wait to write back the g_orig_phy_regsinal value */
	while(!g_done) {
		usleep(1000);
	}
}

static void vblank_handler(int fd, unsigned int frame, unsigned int sec,
			   unsigned int usec, void *data)
{
	drmVBlank vbl;
	struct timeval end;
	vbl_info *info = (vbl_info *)data;

	vbl.request.type = (drmVBlankSeqType) (DRM_VBLANK_RELATIVE | DRM_VBLANK_EVENT);
	vbl.request.sequence = 1;
	vbl.request.signal = (unsigned long)data;

	drmWaitVBlank(g_dev_fd, &vbl);
	gettimeofday(&end, NULL);
	if(info->counter < info->size) {
		info->vsync_array[info->counter++] = TIME_IN_USEC(end);
	}
}

int get_vsync(long *vsync_array, int size)
{
	drmVBlank vbl;
	int ret;
	drmEventContext evctx;
	vbl_info handler_info;

	/* Get current count first */
	vbl.request.type = DRM_VBLANK_RELATIVE;
	vbl.request.sequence = 0;
	ret = drmWaitVBlank(g_dev_fd, &vbl);
	if (ret) {
		ERR("drmWaitVBlank (relative) failed ret: %i\n", ret);
		return -1;
	}

	handler_info.vsync_array = vsync_array;
	handler_info.size = size;
	handler_info.counter = 0;

	/* Queue an event for frame + 1 */
	vbl.request.type = (drmVBlankSeqType) (DRM_VBLANK_RELATIVE | DRM_VBLANK_EVENT);
	vbl.request.sequence = 1;
	vbl.request.signal = (unsigned long)&handler_info;
	ret = drmWaitVBlank(g_dev_fd, &vbl);
	if (ret) {
		ERR("drmWaitVBlank (relative, event) failed ret: %i\n", ret);
		return -1;
	}

	/* Set up our event handler */
	memset(&evctx, 0, sizeof evctx);
	evctx.version = DRM_EVENT_CONTEXT_VERSION;
	evctx.vblank_handler = vblank_handler;
	evctx.page_flip_handler = NULL;

	/* Poll for events */
	for(int i = 0; i < size; i++) {
		struct timeval timeout = { .tv_sec = 3, .tv_usec = 0 };
		fd_set fds;

		FD_ZERO(&fds);
		FD_SET(0, &fds);
		FD_SET(g_dev_fd, &fds);
		ret = select(g_dev_fd + 1, &fds, NULL, NULL, &timeout);

		if (ret <= 0) {
			ERR("select timed out or error (ret %d)\n", ret);
			continue;
		} else if (FD_ISSET(0, &fds)) {
			break;
		}

		ret = drmHandleEvent(g_dev_fd, &evctx);
		if (ret) {
			ERR("drmHandleEvent failed: %i\n", ret);
			return -1;
		}
	}

	return 0;
}
