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
#include "common.h"
#include "dkl.h"
#include "combo.h"


/*******************************************************************************
 * Description
 *  calc_steps_to_sync - This function calculates how many steps we need to take
 *  in order to synchronize the primary and secondary systems given the delta
 *  between primary and secondary and the shift that we need to make in terms of
 *  percentage. Each steps is a single vsync period (typically 16.666 ms).
 * Parameters
 *	double time_diff - The time difference in between the two systems in ms.
 *	double shift - The percentage shift that we need to make in our vsyncs.
 * Return val
 *  int
 ******************************************************************************/
int calc_steps_to_sync(double time_diff, double shift)
{
	return (int) ((time_diff * 100) / shift);
}

#if !TESTING
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
	user_info *ui = (user_info *) si->si_value.sival_ptr;
	if(!ui) {
		return;
	}

	DBG("timer done\n");
	if(ui->get_type() == DKL) {
		dkl_phy_reg *dr = (dkl_phy_reg *) ui->get_reg();
		program_dkl_mmio(dr, 0);
		DBG("DEFAULT VALUES\n dkl_pll_div0 \t 0x%X\n dkl_visa_serializer \t 0x%X\n "
				"dkl_bias \t 0x%X\n dkl_ssc \t 0x%X\n dkl_dco \t 0x%X\n",
				dr->dkl_pll_div0.orig_val,
				dr->dkl_visa_serializer.orig_val,
				dr->dkl_bias.orig_val,
				dr->dkl_ssc.orig_val,
				dr->dkl_dco.orig_val);
		dr->done = 1;
	} else {
		combo_phy_reg *cr = (combo_phy_reg *) ui->get_reg();
		program_combo_mmio(cr, 0);
		DBG("DEFAULT VALUES\n cfgcr0 \t 0x%X\n cfgcr1 \t 0x%X\n",
				cr->cfgcr0.orig_val, cr->cfgcr1.orig_val);
		cr->done = 1;
	}
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
#endif
