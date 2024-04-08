#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "mmio.h"
#include <debug.h>


unsigned char g_raw_device[256] = {0};
gfx_pci_device g_device;
unsigned char *g_mmio = NULL;
unsigned char *g_mem = NULL;
int g_fd = 0;
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

/**
* @brief
* Looks up the main graphics pci device using libpciaccess.
* @param None
* @return The pci device or NULL in case of a failure
*/
struct pci_device *intel_get_pci_device(void)
{
	struct pci_device *pci_dev;
	int error;

	error = pci_system_init();
	if(error) {
		ERR("Couldn't initialize PCI system\n");
		return NULL;
	}

	// Grab the graphics card. Try the canonical slot first, then
	// walk the entire PCI bus for a matching device.
	pci_dev = pci_device_find_by_slot(0, 0, 2, 0);
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
* @param None
* @return device_id (ex 4680 = SUCCESS, 0 = FAILURE)
*/
int get_device_id()
{
	if(!pci_dev) {
		pci_dev = intel_get_pci_device();
	}
	return pci_dev ? pci_dev->device_id : 0;
}

/**
* @brief
* This functions maps the MMIO region
* @param None
* @return
* - 0 = SUCCESS
* - 1 = FAILURE
*/
int map_mmio()
{
	if(!pci_dev) {
		pci_dev = intel_get_pci_device();
	}
	return pci_dev ? intel_mmio_use_pci_bar(pci_dev) : 0;
}


/**
* @brief
* This function can either map MMIO or regular video memory
* @param base_index - Which bar to map
* @param size - The size of memory to map
* @return
* - 0 = SUCCESS
* - 1 = FAILURE
*/
int map_cmn(int base_index, int size)
{
	int found = 0;
	loff_t base;
	int ret_val = 1;
	unsigned long read_size = sizeof(gfx_pci_device);
	const char proc_filename[] = "/proc/bus/pci/00/02.0";
	struct stat file_stats;
	FILE *fp;
	unsigned char **temp;
	size_t bytes_read;

	fp = fopen(proc_filename, "r");

	if(!fp) {

		ERR("Unable to open /proc/bus/pci/00/02.0\n");
		ret_val = 0;

	} else {

		if(lstat(proc_filename, &file_stats) != 0) {

			ERR("Couldn't get file information for"
				"/PROC/bus/pci/00/02.0\n");
			ret_val = 0;

		} else {

			memset(g_raw_device, 0, 256);

			if((unsigned long) file_stats.st_size < sizeof(gfx_pci_device)) {
				read_size = file_stats.st_size;
			}

			bytes_read = fread(g_raw_device, 1, 256, fp);
			if(bytes_read != 256) {
				ERR("Couldn't read 256 bytes of data from PCI space\n");
				ret_val = 0;
			} else {
				memcpy(&g_device, g_raw_device, read_size);
				found = 1;
			}
		} /* end if(lstat...) */

		fclose(fp);

		if(!found) {
			ret_val = 0;
		} else {

			g_fd = open("/dev/mem",O_RDWR);
			if(!g_fd) {
				ERR("Could not open /dev/mem.\n");
				ret_val = 0;
			} else {

				base = (loff_t)(g_device.base_addr[base_index]&0xffffff80000);
				if(base_index == 0) {
					temp = &g_mmio;
					g_mmio_size = size;
				} else {
					temp = &g_mem;
					g_mem_size = size;
				}
				*temp = (unsigned char *) mmap(NULL,
						size, PROT_READ | PROT_WRITE, MAP_SHARED,
						g_fd, base);

				if(*temp == MAP_FAILED) {
					ERR("Unable to mmap GMCH Registers: %s\n", strerror(errno));
					ret_val = 0;
				}
			}
		}
	}
	return ret_val;
}

/**
* @brief
* Unmap the memory range that was mapped during initialization
* @param None
* @return void
*/
void close_mmio_handle()
{
	pci_device_unmap_range(pci_dev, g_mmio, MMIO_SIZE);
	close(g_fd);
}

