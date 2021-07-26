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

int map_mmio()
{
	return map_cmn(MMIO_BAR, MMIO_SIZE);
}

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
			fclose(fp);

		} /* end if(lstat...) */

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
					ERR("Unable to mmap GMCH Registers\n");
					ret_val = 0;
				}
			}
		}
	}
	return ret_val;
}

void close_mmio_handle()
{
	if(g_fd) {
		munmap(g_mmio, g_mmio_size);
		close(g_fd);
	}
}

