// Copyright (C) 2023 Intel Corporation
// SPDX-License-Identifier: MIT

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
#include "mmio.h"
#include "dkl.h"
#include "combo.h"
#include "i915_pciids.h"


phy_funcs phy[] = {
	{"DKL",   dkl_table,   find_enabled_dkl_phys,   program_dkl_phys,   check_if_dkl_done, 0},
	{"COMBO", combo_table, find_enabled_combo_phys, program_combo_phys, check_if_combo_done, 0},
};

ddi_sel adl_s_ddi_sel[] = {
	/*name      de_clk  dpclk               clock_bit   mux_select_low_bit  dpll_num */
	{"DDI_A",   1,      REG(DPCLKA_CFGCR0), 10,         0,                  0, },
	{"DDI_C1",  4,      REG(DPCLKA_CFGCR0), 11,         2,                  0, },
	{"DDI_C2",  5,      REG(DPCLKA_CFGCR0), 24,         4,                  0, },
	{"DDI_C3",  6,      REG(DPCLKA_CFGCR1), 4,          0,                  0, },
	{"DDI_C4",  7,      REG(DPCLKA_CFGCR1), 5,          2,                  0, },
};

ddi_sel tgl_ddi_sel[] = {
	/*name      de_clk  dpclk               clock_bit   mux_select_low_bit  dpll_num */
	{"DDI_A",   1,      REG(DPCLKA_CFGCR0), 10,         0,                  0, },
	{"DDI_B",   2,      REG(DPCLKA_CFGCR0), 11,         2,                  0, },
};


platform platform_table[] = {
	{"TGL",   INTEL_TGL_IDS,  tgl_ddi_sel,   ARRAY_SIZE(tgl_ddi_sel)},
	{"ADL_S", INTEL_ADLS_IDS, adl_s_ddi_sel, ARRAY_SIZE(adl_s_ddi_sel)},
	{"ADL_P", INTEL_ADLP_IDS, tgl_ddi_sel,   ARRAY_SIZE(tgl_ddi_sel)},
};

int g_dev_fd = 0;
int supported_platform = 0;
list<ddi_sel *> *dpll_enabled_list = NULL;

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
	int device_id, i, j;
	if(!IS_INIT()) {
		/* Get the device id of this platform */
		device_id = get_device_id();
		DBG("Device id is 0x%X\n", device_id);
		/* Loop through our supported platform list */
		for(i = 0; i < ARRAY_SIZE(platform_table); i++) {
			for(j = 0; j < MAX_DEVICE_ID; j++) {
				if(platform_table[i].device_ids[j] == device_id) {
					break;
				}
			}
			if(j != MAX_DEVICE_ID) {
				break;
			}
		}
		/* This means we aren't on one of the supported platforms */
		if(i == ARRAY_SIZE(platform_table)) {
			ERR("This platform is not supported. Device id is 0x%X\n", device_id);
			return 1;
		} else {
			supported_platform = i;
		}

		if(map_mmio()) {
			return 1;
		}
		INIT();
	}

	return 0;
}

/*******************************************************************************
 * Description
 *  cleanup_list - This function deallocates all members of the dpll_enabled_list
 * Parameters
 *	NONE
 * Return val
 *  void
 ******************************************************************************/
void cleanup_list()
{
	if(dpll_enabled_list) {
		for(list<ddi_sel *>::iterator it = dpll_enabled_list->begin(); it != dpll_enabled_list->end(); it++) {
			delete *it;
		}
		delete dpll_enabled_list;
	}
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
	cleanup_list();
	close_mmio_handle();
	UNINIT();
}


/*******************************************************************************
 * Description
 *	synchronize_phys - This function finds enabled phys, synchronizes them and
 *	waits for the sync to finish
 * Parameters
 *	int type - Whether it is a DKL or a Combo phy
 *	double time_diff - This is the time difference in between the primary and the
 *	secondary systems in ms. If master is ahead of the slave , then the time
 *	difference is a positive number otherwise negative.
 * Return val
 *	void
 ******************************************************************************/
void synchronize_phys(int type, double time_diff)
{
	if(!phy[type].find()) {
		DBG("No %s PHYs found\n", phy[type].name);
		return;
	}

	/* Cycle through all the phys */
	phy[type].program(time_diff, &phy[type].timer_id);

	/* Wait to write back the original values */
	phy[type].check_if_done();
	timer_delete(phy[type].timer_id);
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

	for(int i = DKL; i < TOTAL_PHYS; i++) {
		synchronize_phys(i, time_diff);
	}
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
 *  pipe_to_wait_for - This function determines the type of vblank synchronization to
 *	use for the output.
 *Parameters
 * int pipe - Indicates which CRTC to get vblank for.  Knowing this, we
 *	can determine which vblank sequence type to use for it.  Traditional
 *	cards had only two CRTCs, with CRTC 0 using no special flags, and
 *	CRTC 1 using DRM_VBLANK_SECONDARY.  The first bit of the pipe
 *	Bits 1-5 of the pipe parameter are 5 bit wide pipe number between
 *	0-31.  If this is non-zero it indicates we're dealing with a
 *	multi-gpu situation and we need to calculate the vblank sync
 *	using DRM_BLANK_HIGH_CRTC_MASK.
 * Return val
 *  int - The flag to OR in for drmWaitVBlank API
 ******************************************************************************/
unsigned int pipe_to_wait_for(int pipe)
{
	int ret = 0;
	if (pipe > 1) {
		ret = (pipe << DRM_VBLANK_HIGH_CRTC_SHIFT) & DRM_VBLANK_HIGH_CRTC_MASK;
	} else if (pipe > 0) {
		ret = DRM_VBLANK_SECONDARY;
	}
	return ret;
}

/*******************************************************************************
 * Description
 *  get_vsync - This function gets a list of vsyncs for the number of times
 *  indicated by the caller and provide their timestamps in the array provided
 * Parameters
 *	long *vsync_array - The array in which vsync timestamps need to be given
 *	int size - The size of this array. This is also the number of times that we
 *	need to get the next few vsync timestamps.
 *	int pipe - This is the pipe whose vblank is needed. Defaults to 0 if not
 *	provided.
 * Return val
 *  int - 0 == SUCCESS, -1 = ERROR
 ******************************************************************************/
int get_vsync(long *vsync_array, int size, int pipe)
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
	handler_info.pipe = pipe;

	/* Queue an event for frame + 1 */
	vbl.request.type = (drmVBlankSeqType)
		(DRM_VBLANK_RELATIVE | DRM_VBLANK_EVENT | pipe_to_wait_for(pipe));
	DBG("vbl.request.type = 0x%X\n", vbl.request.type);
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