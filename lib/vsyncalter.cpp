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

#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <cerrno>
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
#include <xf86drmMode.h>
#include <tgl.h>
#include <adl_s.h>
#include <adl_p.h>
#include "mmio.h"
#include "dkl.h"
#include "combo.h"
#include "i915_pciids.h"

platform platform_table[] = {
	{"TGL",   {INTEL_TGL_IDS},  tgl_ddi_sel,   ARRAY_SIZE(tgl_ddi_sel), 4},
	{"ADL_S_FAM", {INTEL_ADLS_FAM_IDS}, adl_s_ddi_sel, ARRAY_SIZE(adl_s_ddi_sel), 0},
	{"ADL_P_FAM", {INTEL_ADLP_FAM_IDS}, adl_p_ddi_sel, ARRAY_SIZE(adl_p_ddi_sel), 4},
};

int g_dev_fd = 0;
int supported_platform = 0;
list<phys *> *phy_enabled_list = NULL;

int lib_client_done = 0;
/**
* @brief
*	This function creates a timer.
* @param  expire_ms - The time period in ms after which the timer will fire.
* @param *user_ptr - A pointer to pass to the timer handler
* @param *t - A pointer to a pointer where we need to store the timer
* @return
* - 0 == SUCCESS
* - -1 = FAILURE
*/
int phys::make_timer(long expire_ms, void *user_ptr, reset_func reset)
{
	struct sigevent         te;
	struct itimerspec       its;
	struct sigaction        sa;
	int                     sig_no = SIGRTMIN;


	// Set up signal handler
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = reset;
	sigemptyset(&sa.sa_mask);
	if (sigaction(sig_no, &sa, NULL) == -1) {
		ERR("Failed to setup signal handling for timer.\n");
		return -1;
	}

	// Set and enable alarm
	te.sigev_notify = SIGEV_SIGNAL;
	te.sigev_signo = sig_no;
	te.sigev_value.sival_ptr = user_ptr;
	timer_create(CLOCK_REALTIME, &te, &timer_id);

	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = TV_NSEC(expire_ms);
	its.it_value.tv_sec = TV_SEC(expire_ms);
	its.it_value.tv_nsec = TV_NSEC(expire_ms);
	timer_settime(timer_id, 0, &its, NULL);

	return 0;
}

/**
* @brief
* This function opens /dev/dri/card0
* @param None
* @return
* - 0 == SUCCESS
* - <0 == FAILURE
*/
int open_device()
{
	return open("/dev/dri/card0", O_RDWR | O_CLOEXEC, 0);
}

/**
* @brief
* This function closes an open handle
* @param None
* @return void
*/
void close_device()
{
	if(close(g_dev_fd) == -1){
		ERR("Failed to properly close file descriptor. Error: %s\n", strerror(errno));
	}
}

int find_enabled_phys()
{
	int i, j, val, ddi_select;
	reg trans_ddi_func_ctl[] = {
		REG(TRANS_DDI_FUNC_CTL_A),
		REG(TRANS_DDI_FUNC_CTL_B),
		REG(TRANS_DDI_FUNC_CTL_C),
		REG(TRANS_DDI_FUNC_CTL_D),
	};
	phys *new_phy;

	// According to the BSpec:
	// 0000b	None
	// 0001b	DDI A
	// 0010b	DDI B
	// 0011b	DDI C
	// 0100b	DDI USBC1
	// 0101b	DDI USBC2
	// 0110b	DDI USBC3
	// 0111b	DDI USBC4
	// 1000b	DDI USBC5
	// 1001b	DDI USBC6
	// 1000b	DDI D
	// 1001b	DDI E
	// DISPLAY_CCU / DPCLKA Clock	DE Internal Clock
	// DDIA_DE_CLK	                DDIA
	// DDIB_DE_CLK	                USBC1
	// DDII_DE_CLK	                USBC2
	// DDIJ_DE_CLK	                USBC3
	// DDIK_DE_CLK	                USBC4

	phy_enabled_list = new list<phys *>;

	for(i = 0; i < ARRAY_SIZE(trans_ddi_func_ctl); i++) {
		// First read the TRANS_DDI_FUNC_CTL to find if this pipe is enabled or not
		val = READ_OFFSET_DWORD(trans_ddi_func_ctl[i].addr);
		DBG("0x%X = 0x%X\n", trans_ddi_func_ctl[i].addr, val);
		if(!(val & BIT(31))) {
			DBG("Pipe %d is turned off\n", i+1);
			continue;
		}

		// TRANS_DDI_FUNC_CTL bits 30:27 have the DDI which this pipe is connected to
		ddi_select = GETBITS_VAL(val, 30, 27);
		DBG("ddi_select = 0x%X\n", ddi_select);
		for(j = 0; j < platform_table[supported_platform].ds_size; j++) {
			// Match the DDI with the available ones on this platform
			if(platform_table[supported_platform].ds[j].de_clk == ddi_select) {
				switch(platform_table[supported_platform].ds[j].phy) {
					case DKL:
						DBG("Detected a DKL phy on pipe %d\n", i+1);
						new_phy = new dkl(&platform_table[supported_platform].ds[j],
							platform_table[supported_platform].first_dkl_phy_loc);
					break;
					case COMBO:
						DBG("Detected a Combo phy on pipe %d\n", i+1);
						new_phy = new combo(&platform_table[supported_platform].ds[j]);
					break;
					default:
						ERR("Unsupported PHY. Phy is %d\n", platform_table[supported_platform].ds[j].phy);
						return 1;
					break;
				}
				if(!new_phy->is_init()) {
					ERR("PHY not initialized properly\n");
					delete new_phy;
					return 1;
				}
				new_phy->set_pipe(i);
				phy_enabled_list->push_back(new_phy);

				// No point trying to find the same ddi if we have already found it once
				break;
			}
		}
	}
	return 0;
}

/**
* @brief
* This function deallocates all members of the phy_enabled_list
* @param None
* @return void
*/
void cleanup_phy_list()
{
	if(phy_enabled_list) {
		for(list<phys *>::iterator it = phy_enabled_list->begin();
		it != phy_enabled_list->end(); it++) {
			delete *it;
		}
		delete phy_enabled_list;
		phy_enabled_list = NULL;
	}
}

/**
* @brief
* Sets a global flag to indicate client app termination via Ctrl+C
* @param None
* @return void
*/
void shutdown_lib(void)
{
	lib_client_done = 1;
}

/**
* @brief
* Prints the DRM information.	Loops through all CRTCs and Connectors and prints
* their information.
* @param None
* @return void
*/
void print_drm_info(void)
{
	// This list covers most of the connector types that are supported by the DRM
	const char* connector_type_str[] = {
		"Unknown",      // 0
		"VGA",          // 1
		"DVI-I",        // 2
		"DVI-D",        // 3
		"DVI-A",        // 4
		"Composite",    // 5
		"S-Video",      // 6
		"LVDS",         // 7
		"Component",    // 8
		"9PinDIN",      // 9
		"DisplayPort",  // 10
		"HDMI-A",       // 11
		"HDMI-B",       // 12
		"TV",           // 13
		"eDP",          // 14
		"Virtual",      // 15
		"DSI",          // 16
		"DPI",          // 17
		"WriteBack",    // 18
		"SPI",          // 19
		"USB"           // 20
	};

	int fd = open_device();
	if (fd < 0) {
		ERR("Failed to open DRM device: %s\n", strerror(errno));
		return ;
	}

	drmModeRes *resources = drmModeGetResources(fd);
	if (!resources) {
		ERR("drmModeGetResources failed: %s\n", strerror(errno));
		close(fd);
		return;
	}
	INFO("DRM Info:\n");

	// First print Pipe/CRTC info
	INFO("  CRTCs found: %d\n", resources->count_crtcs);
	for (int i = 0; i < resources->count_crtcs; i++) {
		drmModeCrtc *crtc = drmModeGetCrtc(fd, resources->crtcs[i]);
		if (!crtc) {
			ERR("drmModeGetCrtc failed: %s\n", strerror(errno));
			continue;
		}

		double refresh_rate = 0;
		if (crtc->mode_valid) {
			refresh_rate = (double)crtc->mode.clock * 1000.0 / (crtc->mode.vtotal * crtc->mode.htotal);
		}

		INFO("  \tPipe: %2d, CRTC ID: %4d, Mode Valid: %3s, Mode Name: %s, Position: (%4d, %4d), Resolution: %4dx%-4d, Refresh Rate: %.2f Hz\n",
			   i, crtc->crtc_id, (crtc->mode_valid) ? "Yes" : "No", crtc->mode.name,
				crtc->x, crtc->y, crtc->mode.hdisplay, crtc->mode.vdisplay, refresh_rate);

		drmModeFreeCrtc(crtc);
	}

	// Print Connector info
	INFO("  Connectors found: %d\n", resources->count_connectors);
	for (int i = 0; i < resources->count_connectors; i++) {
		drmModeConnector *connector = drmModeGetConnector(fd, resources->connectors[i]);
		if (!connector) {
			ERR("drmModeGetConnector failed: %s\n", strerror(errno));
			continue;
		}

		INFO("  \tConnector: %-4d (ID: %-4d), Type: %-4d (%-12s), Type ID: %-4d, Connection: %-12s\n",
				i, connector->connector_id, connector->connector_type, connector_type_str[connector->connector_type],
				connector->connector_type_id,
				(connector->connection == DRM_MODE_CONNECTED) ? "Connected" : "Disconnected");

		if (connector->connection == DRM_MODE_CONNECTED) {
			drmModeEncoder *encoder = drmModeGetEncoder(fd, connector->encoder_id);
			if (encoder) {
				INFO("\t\t\tEncoder ID: %d, CRTC ID: %d\n", encoder->encoder_id, encoder->crtc_id);
				drmModeFreeEncoder(encoder);
			}
		}
		drmModeFreeConnector(connector);
	}

	drmModeFreeResources(resources);
	close(fd);
}

/**
* @brief
* This function initializes the library. It must be called
* ahead of all other functions because it opens device, maps MMIO space and
* initializes any key global variables.
* @param None
* @return
* - 0 == SUCCESS
* - 1 == FAILURE
*/
int vsync_lib_init()
{
	int device_id, i, j;
	if(!IS_INIT()) {
		// Show Pipe and CRTC info
		print_drm_info();

		// Get the device id of this platform
		device_id = get_device_id();
		DBG("Device id is 0x%X\n", device_id);
		// Loop through our supported platform list
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
		// This means we aren't on one of the supported platforms
		if(i == ARRAY_SIZE(platform_table)) {
			ERR("This platform is not supported. Device id is 0x%X\n", device_id);
			return 1;
		} else {
			supported_platform = i;
		}

		if(map_mmio()) {
			return 1;
		}

		if(find_enabled_phys()) {
			vsync_lib_uninit();
			return 1;
		}

		INIT();
	}

	return 0;
}

/**
* @brief
* This function uninitializes the library by closing devices
* and unmapping memory. It must be called at program exit or else we can have
* memory leaks in the program.
* @param None
* @return void
*/
void vsync_lib_uninit()
{
	cleanup_phy_list();
	close_mmio_handle();
	UNINIT();
}

/**
* @brief
* This function synchronizes the primary and secondary
* systems vsync. It is run on the secondary system. The way that it works is
* that it finds out the default PHY register values, calculates a shift
* based on the time difference provided by the caller and then reprograms the
* PHY registers so that the secondary system can either slow down or speed up
* its vsnyc durations.
* @param time_diff - This is the time difference in between the primary and the
* secondary systems in ms. If master is ahead of the slave , then the time
* difference is a positive number otherwise negative.
* @param pipe - This is the 0 based pipe number to synchronize vsyncs for. Note
* that this variable is optional. So if the caller doesn't provide it or provides
* ALL_PIPES as the value, then all pipes will be synchronized.
* @return void
*/
void synchronize_vsync(double time_diff, int pipe, double shift)
{
	if(!IS_INIT()) {
		ERR("Uninitialized lib, please call lib init first\n");
		return;
	}

	if(phy_enabled_list) {
		for(list<phys *>::iterator it = phy_enabled_list->begin();
			it != phy_enabled_list->end(); it++) {
				if(pipe == ALL_PIPES || pipe == (*it)->get_pipe()) {
					// Program the phy
					(*it)->program_phy(time_diff, shift);
					// Wait until it is done before moving on to the next phy
					(*it)->wait_until_done();
				}
			}
	}
}

/**
* @brief
* The function which will be called whenever a VBLANK occurs
* @param fd - The device file descriptor
* @param frame - Frame number
* @param sec - second when the vblank occured
* @param usec - micro second when the vblank occured
* @param *data - a private data structure pointing to the vbl_info
* @return void
*/
static void vblank_handler(int fd, unsigned int frame, unsigned int sec,
			   unsigned int usec, void *data)
{
	drmVBlank vbl;
	vbl_info *info = (vbl_info *)data;
	memset(&vbl, 0, sizeof(drmVBlank));
	if(info->counter < info->size) {
		info->vsync_array[info->counter++] = TIME_IN_USEC(sec, usec);
	}

	vbl.request.type = (drmVBlankSeqType) (DRM_VBLANK_RELATIVE | DRM_VBLANK_EVENT |
		pipe_to_wait_for(info->pipe));
	vbl.request.sequence = 1;
	vbl.request.signal = (unsigned long)data;

	drmWaitVBlank(g_dev_fd, &vbl);
}

/**
* @brief
* This function determines the type of vblank synchronization to
* use for the output.
* @param pipe - Indicates which CRTC to get vblank for.  Knowing this, we
* can determine which vblank sequence type to use for it.  Traditional
* cards had only two CRTCs, with CRTC 0 using no special flags, and
* CRTC 1 using DRM_VBLANK_SECONDARY.  The first bit of the pipe
* Bits 1-5 of the pipe parameter are 5 bit wide pipe number between
* 0-31.  If this is non-zero it indicates we're dealing with a
* multi-gpu situation and we need to calculate the vblank sync
* using DRM_BLANK_HIGH_CRTC_MASK.
* @return The flag to OR in for drmWaitVBlank API
*/
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

/**
* @brief
* This function gets a list of vsyncs for the number of times
* indicated by the caller and provide their timestamps in the array provided
* @param *vsync_array - The array in which vsync timestamps need to be given
* @param size - The size of this array. This is also the number of times that we
* need to get the next few vsync timestamps.
* @param pipe - This is the pipe whose vblank is needed. Defaults to 0 if not
* provided.
* @return
* - 0 == SUCCESS
* - 1 = ERROR
*/
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

	// Queue an event for frame + 1
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

	// Set up our event handler
	memset(&evctx, 0, sizeof evctx);
	evctx.version = DRM_EVENT_CONTEXT_VERSION;
	evctx.vblank_handler = vblank_handler;
	evctx.page_flip_handler = NULL;

	// Poll for events
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


/**
 * @brief
 * This function collects vblank timestamps and prints average interval between them
 *
 * @param pipe
 */
double get_vblank_interval(int pipe)
{
	const int MAX_STAMPS=20;
	long timestamps[MAX_STAMPS];

	if (get_vsync(timestamps, MAX_STAMPS, pipe) == 0) {
			long total_interval = 0;
			for (int i = 0; i < MAX_STAMPS - 1; ++i) {
					total_interval += (timestamps[i+1] - timestamps[i]);
			}

			double avg_interval =  total_interval / (MAX_STAMPS - 1) / 1000.0; // Convert to milliseconds
			return avg_interval;
	}
	return 0.0;
}

