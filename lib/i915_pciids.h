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

#ifndef _I915_PCI_IDS_H
#define _I915_PCI_IDS_H

#define INTEL_TGL_IDS \
    0x9A60, \
    0x9A68, \
    0x9A70, \
    0x9A40, \
    0x9A49, \
    0x9A59, \
    0x9A78, \
    0x9AC0, \
    0x9AC9, \
    0x9AD9, \
    0x9AF8


#define INTEL_ADL_S_IDS \
    0x4680, \
    0x4682, \
    0x4688, \
    0x468A, \
    0x468B, \
    0x4690, \
    0x4692, \
    0x4693, \
    0x4680, \
    0x4682, \
    0x4692, \
    0x4693

#define INTEL_ADL_P_IDS \
    0x46A0, \
    0x46A1, \
    0x46A2, \
    0x46A3, \
    0x46A6, \
    0x46A8, \
    0x46AA, \
    0x462A, \
    0x4626, \
    0x4628, \
    0x46B0, \
    0x46B1, \
    0x46B2, \
    0x46B3, \
    0x46C0, \
    0x46C1, \
    0x46C2, \
    0x46C3

#define INTEL_RPL_P_IDS \
    0xA7A0, \
    0xA7A0, \
    0xA7A8

#define INTEL_RPL_S_IDS \
    0x4680, \
    0x4682, \
    0x4692, \
    0x4693, \
    0xA780, \
    0xA782, \
    0xA783

#define INTEL_RPL_H_IDS \
    0x4688, \
    0x468B, \
    0xA788, \
    0xA78B, \
    0xA7A0, \
    0xA7A8

#define INTEL_ADL_P_FAM_IDS \
    INTEL_ADL_P_IDS, \
    INTEL_RPL_P_IDS, \
    INTEL_RPL_H_IDS

#define INTEL_ADL_S_FAM_IDS \
    INTEL_ADL_S_IDS, \
    INTEL_RPL_S_IDS

#define INTEL_MTL_M_IDS \
    0x7D40, \
    0x7D41, \
    0x7D60, \
    0x7D67

#define INTEL_MTL_P_IDS \
    0x7D45, \
    0x7D55, \
    0x7DD5

#define INTEL_ARL_P_IDS \
    0x7D51, \
    0x7DD1

#define INTEL_MTL_FAM_IDS \
    INTEL_MTL_M_IDS, \
    INTEL_MTL_P_IDS, \
    INTEL_ARL_P_IDS

#endif
