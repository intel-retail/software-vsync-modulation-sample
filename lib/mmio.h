// Copyright (C) 2023 Intel Corporation
// SPDX-License-Identifier: MIT

#ifndef _MMIO_H
#define _MMIO_H

#include <pciaccess.h>

typedef struct _gfx_pci_device {
    unsigned short vendor_id, device_id;    /* Identity of the device */
    unsigned short device_class;            /* PCI device class */
    unsigned long long irq;     /* IRQ number */
    pciaddr_t base_addr[6];     /* Base addresses */
    pciaddr_t size[6];          /* Region sizes */
    pciaddr_t rom_base_addr;    /* Expansion ROM base address */
    pciaddr_t rom_size;         /* Expansion ROM size */

} gfx_pci_device;

#define MMIO_SIZE 2*1024*1024
#define MMIO_BAR  0
#define READ_OFFSET_DWORD(b, x) (*((volatile uint32_t *) (b + x + cpu_offset)))
#define WRITE_OFFSET_DWORD(b, x, y) ((*((volatile uint32_t *) (b + x + cpu_offset))) = y)
#define IS_INIT() g_init

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

#endif
