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

#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <vsyncalter.h>
#include <debug.h>
#include <math.h>
#include "version.h"

using namespace std;

/**
* @brief
* This function finds the average of all the vertical syncs that
* have been provided by the primary system to the secondary.
* @param *va - The array holding all the vertical syncs of the primary system
* @param sz - The size of this array
* @return The average of the vertical syncs
*/
long find_avg(long *va, int sz)
{
	int avg = 0;
	for(int i = 0; i < sz - 1; i++) {
		avg += va[i+1] - va[i];
	}
	return avg / ((sz == 1) ? sz : (sz - 1));
}

/**
* @brief
* This function prints out the last N vsyncs that the system has
* received either on its own or from another system. It will only print in DBG
* mnode
* @param *msg - Any prefix message to be printed.
* @param *va - The array of vsyncs
* @param sz - The size of this array
* @return void
*/
void print_vsyncs(char *msg, long *va, int sz)
{
	INFO("%s VSYNCS\n", msg);
	for(int i = 0; i < sz; i++) {
		INFO("%6f\n", ((double) va[i])/1000000);
	}
}

/**
* @brief
* This is the main function
* @param argc - The number of command line arguments
* @param *argv[] - Each command line argument in an array
* @return
* - 0 = SUCCESS
* - 1 = FAILURE
*/
int main(int argc, char *argv[])
{
	int ret = 0, size;
	long *client_vsync, avg;
	
	INFO("Vbltest Version: %s\n", get_version().c_str());

	if(argc != 3) {
		ERR("Usage: %s <number of vsyncs to get timestamp for> <0 based pipe to get for>\n", argv[0]);
		return 1;
	}

	size = atoi(argv[1]);
	if(size < 0 || size > 6000) {
		ERR("Specify the number of vsyncs to be between 1 and 600\n");
		return 1;
	}

	if(vsync_lib_init()) {
		ERR("Couldn't initialize vsync lib\n");
		return 1;
	}

	client_vsync = new long[size];

	if(get_vsync(client_vsync, size, atoi(argv[2]))) {
		delete [] client_vsync;
		vsync_lib_uninit();
		return 1;
	}

	avg = find_avg(&client_vsync[0], size);
	print_vsyncs((char *) "", client_vsync, size);
	INFO("Time average of the vsyncs on the primary system is %ld\n", avg);

	delete [] client_vsync;
	vsync_lib_uninit();
	return ret;
}
