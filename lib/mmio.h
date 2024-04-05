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

#ifndef _MMIO_H
#define _MMIO_H

#include <pciaccess.h>

typedef struct _gfx_pci_device {
    unsigned short vendor_id, device_id;    // Identity of the device
    unsigned short device_class;            // PCI device class
    unsigned long long irq;     // IRQ number
    pciaddr_t base_addr[6];     // Base addresses
    pciaddr_t size[6];          // Region sizes
    pciaddr_t rom_base_addr;    // Expansion ROM base address
    pciaddr_t rom_size;         // Expansion ROM size

} gfx_pci_device;

#define MMIO_SIZE 2*1024*1024
#define MMIO_BAR  0
#define READ_OFFSET_DWORD(x) (*((volatile uint32_t *) (g_mmio + x + cpu_offset)))
#define WRITE_OFFSET_DWORD(x, y) ((*((volatile uint32_t *) (g_mmio + x + cpu_offset))) = y)
#define IS_INIT() g_init
#define INIT()    g_init = 1;
#define UNINIT()    g_init = 0;

extern unsigned int cpu_offset;
extern unsigned char g_raw_device[256];
extern gfx_pci_device g_device;
extern unsigned char *g_mmio;
extern unsigned char *g_mem;
extern int g_mmio_size;
extern int g_mem_size;
extern int g_fd;
extern int g_drm_fd;
extern int g_init;

int map_mmio();
int map_cmn(int base_index, int size);
void close_mmio_handle();
int get_device_id();

#endif
