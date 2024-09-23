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
 * Print help message
 *
 * @param program_name - Name of the program
 * @return void
 */
void print_help(const char *program_name)
{
	PRINT("Usage: %s [-p pipe] [-c vsync_count] [-v loglevel] [-h]\n"
		"Options:\n"
		"  -p pipe        Pipe to get stamps for.  0,1,2 ... (default: 0)\n"
		"  -c vsync_count Number of vsyncs to get timestamp for (default: 300)\n"
		"  -v loglevel    Log level: error, warning, info, debug or trace (default: info)\n"
		"  -h             Display this help message\n",
		program_name);
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
	int ret = 0;
	long *client_vsync, avg;

	INFO("Vbltest Version: %s\n", get_version().c_str());
	int vsync_count = 300;  // number of vsyncs to get timestamp for
	int pipe = 0;  // Default pipe# 0
	int opt;
	while ((opt = getopt(argc, argv, "p:c:v:h")) != -1) {
		switch (opt) {
			case 'p':
				pipe = std::stoi(optarg);
				break;
			case 'c':
				vsync_count = std::stoi(optarg);
				break;
			case 'v':
				set_log_level(optarg);
				break;
			case 'h':
			case '?':
				if (optopt == 'p' || optopt == 'c' || optopt == 'v') {
					ERR("Option -%c requires an argument.\n", char(optopt));
				} else {
					ERR("Unknown option: -%c\n", char(optopt));
				}
				print_help(argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	if(vsync_lib_init()) {
		ERR("Couldn't initialize vsync lib\n");
		return 1;
	}

	client_vsync = new long[vsync_count];

	if(get_vsync(client_vsync, vsync_count, pipe)) {
		delete [] client_vsync;
		vsync_lib_uninit();
		return 1;
	}

	avg = find_avg(&client_vsync[0], vsync_count);
	print_vsyncs((char *) "", client_vsync, vsync_count);
	INFO("Time average of the vsyncs on the primary system is %ld\n", avg);

	delete [] client_vsync;
	vsync_lib_uninit();
	return ret;
}
