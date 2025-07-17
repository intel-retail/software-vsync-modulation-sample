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
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <cerrno>
#include <mutex>
#include <limits.h>
#include "mmio.h"
#include <debug.h>


unsigned char g_raw_device[256] = {0};
gfx_pci_device g_device;
unsigned char *g_mmio = NULL;
unsigned char *g_mem = NULL;
int g_fd = -1;
int g_mmio_size = 0;
int g_mem_size = 0;
unsigned int cpu_offset=0;
int g_init = 0;

const int order = 32;
const unsigned long polynom = 0x4c11db7;
const int direct = 1;
const unsigned long crcinit = 0xffffffff;
const unsigned long crcxor = 0xffffffff;
const int refin = 1;
const int refout = 1;

unsigned long crcmask;
unsigned long crchighbit;
unsigned long crcinit_direct;
unsigned long crcinit_nondirect;
unsigned long crctab[256];
struct pci_device *pci_dev = NULL;
std::mutex map_mutex; // Global mutex to protect map_cmn

/**
* @brief
* Looks up the main graphics pci device using libpciaccess.
* @param device_str - The device string to look up
* @return The pci device or NULL in case of a failure
*/
struct pci_device *intel_get_pci_device(const char *device_str)
{
	struct pci_device *pci_dev;
	int error;
	char sysfs_device_path[PATH_MAX];
	char resolved_path[PATH_MAX];
	unsigned int domain, bus, dev, func;

	if (!device_str) {
		ERR("Device string is NULL\n");
		return NULL;
	}

	// Construct the sysfs device path
	snprintf(sysfs_device_path, sizeof(sysfs_device_path), "/sys/class/drm/%s/device", strrchr(device_str, '/') + 1);

	// Resolve the symlink to something like "/sys/devices/pci0000:00/0000:00:02.0"
	if (realpath(sysfs_device_path, resolved_path) == NULL) {
		ERR("Failed to resolve device symlink");
		return NULL;
	}

	// Parse the PCI domain:bus:device.function (e.g., "0000:00:02.0")
	const char* pci_info = strrchr(resolved_path, '/');
	if (!pci_info) {
		ERR("Invalid resolved path: %s\n", resolved_path);
		return NULL;
	}

	pci_info++; // Skip the '/' to get to the actual "0000:00:02.0"

	if (sscanf(pci_info, "%x:%x:%x.%x", &domain, &bus, &dev, &func) != 4) {
		ERR("Failed to parse PCI info from '%s'\n", pci_info);
		return NULL;
	}

	error = pci_system_init();
	if(error) {
		ERR("Couldn't initialize PCI system\n");
		return NULL;
	}

	// Find the PCI device using the parsed domain, bus, device, and function
	pci_dev = pci_device_find_by_slot(domain, bus, dev, func);
	if (pci_dev == NULL || pci_dev->vendor_id != 0x8086) {
		struct pci_device_iterator *iter;
		struct pci_id_match match;

		match.vendor_id = 0x8086; // Intel
		match.device_id = PCI_MATCH_ANY;
		match.subvendor_id = PCI_MATCH_ANY;
		match.subdevice_id = PCI_MATCH_ANY;

		match.device_class = 0x3 << 16;
		match.device_class_mask = 0xff << 16;

		match.match_data = 0;

		iter = pci_id_match_iterator_create(&match);
		pci_dev = pci_device_next(iter);
		pci_iterator_destroy(iter);
	}

	if(!pci_dev) {
		ERR("Couldn't find Intel graphics card\n");
		return NULL;
	}

	error = pci_device_probe(pci_dev);
	if(error) {
		ERR("Couldn't probe graphics card\n");
		return NULL;
	}

	if (pci_dev->vendor_id != 0x8086) {
		ERR("Graphics card is non-intel\n");
		return NULL;
	}

	return pci_dev;
}

/**
* @brief
* Fill a mmio_data stucture with igt_mmio to point at the mmio bar.
* @param *pci_dev - intel graphics pci device
* @return
* - 0 = SUCCESS
* - 1 = FAILURE
*/
int intel_mmio_use_pci_bar(struct pci_device *pci_dev)
{
	int mmio_bar, mmio_size;
	int error;

	mmio_bar = 0;
	mmio_size = MMIO_SIZE;

	error = pci_device_map_range(pci_dev,
					  pci_dev->regions[mmio_bar].base_addr,
					  mmio_size,
					  PCI_DEV_MAP_FLAG_WRITABLE,
					  (void **) &g_mmio);

	if(error) {
		ERR("Couldn't map MMIO region\n");
		return 1;
	}
	return 0;
}

/**
* @brief
* This functions gets the deivce ID of the device
* @param device_str - The device string to look up
* @return device_id (ex 4680 = SUCCESS, 0 = FAILURE)
*/
int get_device_id(const char *device_str)
{
	if(!pci_dev) {
		pci_dev = intel_get_pci_device(device_str);
	}
	return pci_dev ? pci_dev->device_id : 0;
}

/**
* @brief
* This functions maps the MMIO region
* @param device_str - The device string to look up
* @return
* - 0 = SUCCESS
* - 1 = FAILURE
*/
int map_mmio(const char *device_str)
{
	if(!pci_dev) {
		pci_dev = intel_get_pci_device(device_str);
	}
	return pci_dev ? intel_mmio_use_pci_bar(pci_dev) : 0;
}

/**
* @brief
* Unmap the memory range that was mapped during initialization
* @param None
* @return int, 0 - success, non zero - failure
*/
int close_mmio_handle()
{
    std::lock_guard<std::mutex> lock(map_mutex); // Lock the mutex for the scope of the function

    int status = 0;

    if (pci_dev && g_mmio) {
        if (pci_device_unmap_range(pci_dev, g_mmio, MMIO_SIZE) != 0) {
            ERR("Failed to unmap MMIO range.\n");
            status = 1;
        }
    }

    pci_dev = NULL;

    pci_system_cleanup(); // no error return, assumed to succeed

    if (g_fd >= 0) {
        if (close(g_fd) == -1) {
            ERR("Failed to properly close file descriptor. Error: %s\n", strerror(errno));
            status = 1;
        }
        g_fd = -1;
    }

    return status;
}
