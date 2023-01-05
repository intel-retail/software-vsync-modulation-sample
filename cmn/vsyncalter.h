/*INTEL CONFIDENTIAL
Copyright (C) 2022 Intel Corporation
This software and the related documents are Intel copyrighted materials, and your use of them is governed by the express license under which they were provided to you ("License"). Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties, other than those that are expressly stated in the License.
*/
#ifndef _VSYNCALTER_H
#define _VSYNCALTER_H

#define ONE_VSYNC_PERIOD_IN_MS        16.666
#define MAX_TIMESTAMPS                100

int vsync_lib_init();
void vsync_lib_uninit();
void synchronize_vsync(double time_diff);
int get_vsync(long *vsync_array, int size);
int find_enabled_combo_phys();

#endif
