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
#include <xf86drm.h>
#include <list>
#include "common.h"
#include "mmio.h"
#include "i915_pciids.h"

using namespace std;

trans_reg pipe_reg_table[] = {
	{REG(TRANS_VBLANK_A),   REG(TRANS_VTOTAL_A),   0 , 0, 0},
	{REG(TRANS_VBLANK_B),   REG(TRANS_VTOTAL_B),   0 , 0, 0},
	{REG(TRANS_VBLANK_C),   REG(TRANS_VTOTAL_C),   0 , 0, 0},
	{REG(TRANS_VBLANK_D),   REG(TRANS_VTOTAL_D),   0 , 0, 0},
	{REG(TRANS_VBLANK_EDP), REG(TRANS_VTOTAL_EDP), 0 , 0, 0},
};

int g_dev_fd = 0;

/*******************************************************************************
 * Description
 *  open_device - This function opens /dev/dri/card0
 * Parameters
 *	NONE
 * Return val
 *  int - >=0 == SUCCESS, <0 == FAILURE
 ******************************************************************************/
int open_device()
{
	return open("/dev/dri/card0", O_RDWR | O_CLOEXEC, 0);
}

/*******************************************************************************
 * Description
 *  close_device - This function closes an open handle
 * Parameters
 *	NONE
 * Return val
 *  void
 ******************************************************************************/
void close_device()
{
	close(g_dev_fd);
}

/*******************************************************************************
 * Description
 *  vsync_lib_init - This function initializes the library. It must be called
 *	ahead of all other functions because it opens device, maps MMIO space and
 *	initializes any key global variables.
 * Parameters
 *	NONE
 * Return val
 *  int - 0 == SUCCESS, 1 == FAILURE
 ******************************************************************************/
int vsync_lib_init()
{
	if(!IS_INIT()) {
		if(map_mmio()) {
			return 1;
		}
		INIT();
	}

	return 0;
}

/*******************************************************************************
 * Description
 *  vsync_lib_uninit - This function uninitializes the library by closing devices
 *  and unmapping memory. It must be called at program exit or else we can have
 *  memory leaks in the program.
 * Parameters
 *	NONE
 * Return val
 *  void
 ******************************************************************************/
void vsync_lib_uninit()
{
	close_mmio_handle();
	UNINIT();
}

/*******************************************************************************
 * Description
 *	print_time - This function prints the current time
 * Parameters
 *	NONE
 * Return val
 *	void
 ******************************************************************************/
static void print_time()
{
	struct timeval t;
	gettimeofday(&t,NULL);
	DBG("Time is %ld.%ld\n", t.tv_sec, t.tv_usec);
}

/*******************************************************************************
 * Description
 *	pipe_find - This function finds the number of pipes that are currently
 *	enabled. It does so by going through a list of Vblank and Vtotal registers
 *	for currently known pipes for all current platforms and checks if they have a
 *	valid value in them. It further checks if the vertical total is at least one
 *	more than vertical blank because that is what we will be manipulating to
 *	synchronize the vsyncs.
 * Parameters
 *	NONE
 * Return val
 *	int - The total number of pipes that were found to be enabled
 ******************************************************************************/
int pipe_find()
{
	unsigned int vblank, vtotal, active, total;
	int enabled = 0;
	TRACING();

	for(int i = 0; i < ARRAY_SIZE(pipe_reg_table); i++) {
		vblank = READ_OFFSET_DWORD(pipe_reg_table[i].vblank.addr);
		/* vblank must have a valid value in it */
		if(vblank != 0 && vblank != 0xFFFFFFFF) {
			vtotal = READ_OFFSET_DWORD(pipe_reg_table[i].vtotal.addr);
			active = GETBITS_VAL(vtotal, 12, 0);
			total = GETBITS_VAL(vtotal, 28, 16);
			/* Total must be at least one more than active */
			if(total > active+1) {
				pipe_reg_table[i].enabled = 1;
				enabled++;
				DBG("Pipe #%d is on\n", i);
			} else {
				ERR("VTOTAL doesn't have even one line after VACTIVE. Vtotal = %d, Vactive = %d\n", total, active);
			}
		}
	}
	DBG("Total pipes on: %d\n", enabled);
	return enabled;
}

/*******************************************************************************
 * Description
 *	calc_reg_vals - This function finds out the current and to be altered values
 *	for a register (Vblank or Vtotal)
 * Parameters
 *	reg *r - The register pointer with the address and whose current and mod
 *	values need to be stored
 *	int shift - A positive or negative shift to be made to the register
 * Return val
 *	void
 ******************************************************************************/
void calc_reg_vals(reg *r, int shift)
{
	unsigned int total;
	r->orig_val = r->mod_val = READ_OFFSET_DWORD(r->addr);
	total = GETBITS_VAL(r->orig_val, 28, 16);
	total -= shift;
	r->mod_val &= ~GENMASK(28, 16);
	r->mod_val |= total<<16;
}

/*******************************************************************************
 * Description
 *	program_regs - This function writes either the original or the altered value
 *	of the vblank and vtotal registers.
 * Parameters
 *	trans_reg *tr - The register whose value needs to be written
 *	int mod - 0 = write original value, 1 = write modified value
 * Return val
 *	void
 ******************************************************************************/
void program_regs(trans_reg *tr, int mod)
{
	DBG("Writing to Vblank addr = 0x%X, with a value of 0x%X\n",
		tr->vblank.addr, mod ? tr->vblank.mod_val : tr->vblank.orig_val);
	WRITE_OFFSET_DWORD(tr->vblank.addr, mod ? tr->vblank.mod_val : tr->vblank.orig_val);
	DBG("Writing to Vtotal addr = 0x%X, with a value of 0x%X\n",
		tr->vtotal.addr, mod ? tr->vtotal.mod_val : tr->vtotal.orig_val);
	WRITE_OFFSET_DWORD(tr->vtotal.addr, mod ? tr->vtotal.mod_val : tr->vtotal.orig_val);
}

/*******************************************************************************
 * Description
 *	pipe_program - This function programs the pipe register. It does so by first
 *	going through all the pipe registers that are currently enabled. It then
 *	calculates how much of a shift do we need to make and for how long. It then
 *	sets a timer for the time period after which we need to reprogram the default
 *	value and finally programs the modified value.
 * Parameters
 *	double time_diff - The time difference between two systems (in ms) that we
 *	need to synchronize
 * Return val
 *	void
 ******************************************************************************/
void pipe_program(double time_diff)
{
	int shift = 1;
	int expire_ms;
	TRACING();

	for(int i = 0; i < ARRAY_SIZE(pipe_reg_table); i++) {
		/* Skip any that aren't enabled */
		if(!pipe_reg_table[i].enabled) {
			continue;
		}

		pipe_reg_table[i].done = 0;

		if(time_diff < 0) {
			shift *= -1;
		}
		calc_reg_vals(&pipe_reg_table[i].vblank, shift);
		calc_reg_vals(&pipe_reg_table[i].vtotal, shift);

		expire_ms = calc_time_to_sync(pipe_reg_table[i].vblank.orig_val, time_diff);
		DBG("Timer expiry in ms %d\n", expire_ms);

		user_info *ui = new user_info(&pipe_reg_table[i]);
		make_timer((long) expire_ms, ui, &pipe_reg_table[i].timer_id);

		program_regs(&pipe_reg_table[i], 1);
		print_time();
	}
}

/*******************************************************************************
 * Description
 *	pipe_check_if_done - This function checks to see if the time period for
 *	synchronizing two systems is over and if so, it deletes the associated timer
 * Parameters
 *	NONE
 * Return val
 *	void
 ******************************************************************************/
void pipe_check_if_done()
{
	TRACING();
	for (int i = 0; i < ARRAY_SIZE(pipe_reg_table); i++) {
		if (pipe_reg_table[i].enabled) {
			while(!pipe_reg_table[i].done) {
				usleep(1000);
			}
			timer_delete(pipe_reg_table[i].timer_id);
		}
	}
}

/*******************************************************************************
 * Description
 *  synchronize_vsync - This function synchronizes the primary and secondary
 *  systems vsync. It is run on the secondary system. The way that it works is
 *  that it finds out the default PHY register values, calculates a shift
 *  based on the time difference provided by the caller and then reprograms the
 *  PHY registers so that the secondary system can either slow down or speed up
 *  its vsnyc durations.
 * Parameters
 * double time_diff - This is the time difference in between the primary and the
 * secondary systems in ms. If master is ahead of the slave , then the time
 * difference is a positive number otherwise negative.
 * Return val
 *  void
 ******************************************************************************/
void synchronize_vsync(double time_diff)
{
	if(!IS_INIT()) {
		ERR("Uninitialized lib, please call lib init first\n");
		return;
	}

	if(!pipe_find()) {
		DBG("No pipes that are enabled found\n");
		return;
	}
	pipe_program(time_diff);
	pipe_check_if_done();
}

/*******************************************************************************
 * Description
 *  calc_time_to_sync - This function calculates how many ms we need to take
 *  in order to synchronize the primary and secondary systems given the delta
 *  between primary and secondary and the shift that we need to make in terms of
 *  percentage.
 * Parameters
 *	int vblank - The vblank register's value where active bits are stored
 *	double time_diff - The time difference in between the two systems in ms.
 * Return val
 *  int - The ms it will take to sync the systems
 ******************************************************************************/
int calc_time_to_sync(int vblank, double time_diff)
{
	unsigned int active = GETBITS_VAL(vblank, 12, 0) + 1;

	return abs(time_diff * active);
}

/*******************************************************************************
 * Description
 *	timer_handler - The timer callback function which gets executed whenever a
 *	timer expires. We program MMIO registers of the PHY in this function becase
 *	we have waited for a certain time period to get the primary and secondary
 *	systems vsync in sync and now it is time to reprogram the default values
 *	for the secondary system's PHYs.
 * Parameters
 *	int sig - The signal that fired
 *	siginfo_t *si - A pointer to a siginfo_t, which is a structure containing
    further information about the signal
 *	void *uc - This is a pointer to a ucontext_t structure, cast to void *.
    The structure pointed to by this field contains signal context information
	that was saved on the user-space stack by the kernel
 * Return val
 *  void
 ******************************************************************************/
void timer_handler(int sig, siginfo_t *si, void *uc)
{
	trans_reg *tr;
	user_info *ui = (user_info *) si->si_value.sival_ptr;
	if(!ui) {
		return;
	}
	print_time();
	DBG("timer done\n");
	tr = (trans_reg *) ui->get_reg();
	program_regs(tr, 0);
	tr->done = 1;
	delete ui;
}

/*******************************************************************************
 * Description
 *	make_timer - This function creates a timer.
 * Parameters
 *	long expire_ms - The time period in ms after which the timer will fire.
 *	void *user_ptr - A pointer to pass to the timer handler
 *	timer_t *t     - A pointer to a pointer where we need to store the timer
 * Return val
 *	int - 0 == SUCCESS, -1 = FAILURE
 ******************************************************************************/
int make_timer(long expire_ms, void *user_ptr, timer_t *t)
{
	struct sigevent         te;
	struct itimerspec       its;
	struct sigaction        sa;
	int                     sig_no = SIGRTMIN;


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
	te.sigev_value.sival_ptr = user_ptr;
	timer_create(CLOCK_REALTIME, &te, t);

	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = TV_NSEC(expire_ms);
	its.it_value.tv_sec = TV_SEC(expire_ms);
	its.it_value.tv_nsec = TV_NSEC(expire_ms);
	timer_settime(*t, 0, &its, NULL);

	return 0;
}

/*******************************************************************************
 * Description
 *	vblank_handler - The function which will be called whenever a VBLANK occurs
 * Parameters
 *	int fd - The device file descriptor
 *	unsigned int frame - Frame number
 *	unsigned int sec - second when the vblank occured
 *	unsigned int usec - micro second when the vblank occured
 *	void *data - a private data structure pointing to the vbl_info
 * Return val
 *  void
 ******************************************************************************/
static void vblank_handler(int fd, unsigned int frame, unsigned int sec,
			   unsigned int usec, void *data)
{
	drmVBlank vbl;
	vbl_info *info = (vbl_info *)data;
	memset(&vbl, 0, sizeof(drmVBlank));
	if(info->counter < info->size) {
		info->vsync_array[info->counter++] = TIME_IN_USEC(sec, usec);
	}

	vbl.request.type = (drmVBlankSeqType) (DRM_VBLANK_RELATIVE | DRM_VBLANK_EVENT);
	vbl.request.sequence = 1;
	vbl.request.signal = (unsigned long)data;

	drmWaitVBlank(g_dev_fd, &vbl);
}

/*******************************************************************************
 * Description
 *  get_vsync - This function gets a list of vsyncs for the number of times
 *  indicated by the caller and provide their timestamps in the array provided
 * Parameters
 *	long *vsync_array - The array in which vsync timestamps need to be given
 *	int size - The size of this array. This is also the number of times that we
 *	need to get the next few vsync timestamps.
 * Return val
 *  int - 0 == SUCCESS, -1 = ERROR
 ******************************************************************************/
int get_vsync(long *vsync_array, int size)
{
	drmVBlank vbl;
	int ret;
	drmEventContext evctx;
	vbl_info handler_info;

	g_dev_fd = open_device();
	if(g_dev_fd < 0) {
		ERR("Couldn't open /dev/dri/card0. Is i915 installed?\n");
		return -1;
	}

	memset(&vbl, 0, sizeof(drmVBlank));

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
		close_device();
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
		}

		ret = drmHandleEvent(g_dev_fd, &evctx);
		if (ret) {
			ERR("drmHandleEvent failed: %i\n", ret);
			close_device();
			return -1;
		}
	}

	close_device();
	return 0;
}
