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

#ifndef _VSYNCALTER_H
#define _VSYNCALTER_H

#include <stdint.h>
#include <stdbool.h>
#include <debug.h>

#define VSYNC_ONE_VSYNC_PERIOD_IN_MS        16.666
#define VSYNC_MAX_TIMESTAMPS                100
#define VSYNC_ALL_PIPES                     4
#define VSYNC_DEFAULT_DEVICE                "/dev/dri/card0"
#define VSYNC_DEFAULT_PIPE                  0
#define VSYNC_DEFAULT_SHIFT                 0.01
#define VSYNC_DEFAULT_SHIFT2                0.1
#define VSYNC_DEFAULT_RESET                 true
#define VSYNC_DEFAULT_COMMIT                true
#define VSYNC_DEFAULT_WAIT_IN_MS            50
#define VSYNC_TIME_DELTA_FOR_STEP           1000

#ifdef __cplusplus
extern "C" {
#endif

int vsync_lib_init(const char *device_str, bool dp_m_n);
int vsync_lib_uninit();
int synchronize_vsync(double time_diff, int pipe, double shift, double shift2,
						int step_threshold, int wait_between_steps, bool reset,
						bool commit);
int get_vsync(const char *device_str, uint64_t *vsync_array, int size, int pipe);
double get_vblank_interval(const char *device_str, int pipe, int size);
int set_pll_clock(double pll_clock, int pipe, double shift,
						uint32_t wait_between_steps);
double get_pll_clock(int pipe);
bool get_phy_name(int pipe, char* out_name, size_t out_size);
int print_drm_info(const char *device_str);
void shutdown_lib(void);
const char* find_first_dri_card(void);
int set_log_mode(const char* mode);
int set_log_level(log_level level);
int set_log_level_str(const char* log_level);

#ifdef __cplusplus
}
#endif

#endif
