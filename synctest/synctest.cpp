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
int thread_continue = 1;

/**
 * @brief
 * This function is the background thread task to call print_vblank_interval
 *
 * @param arg
 * @return void*
 */
void* background_task(void *arg)
{
	const int WAIT_TIME_IN_MICRO = 1000*1000;  // 1 sec
	int pipe = *(int*)arg;
	double avg_interval;

	INFO("VBlank interval during synchronization ->\n");
	// Thread runs this loop until it is canceled
	while (thread_continue) {
		usleep(WAIT_TIME_IN_MICRO);
		avg_interval = get_vblank_interval(pipe);
		INFO("\t\tVBlank interval on pipe %d is %.4lf ms\n", pipe, avg_interval);
	}
	return NULL;
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
	PRINT("Usage: %s [-p pipe] [-d delta] [-s shift] [-v loglevel] [-h]\n"
		"Options:\n"
		"  -p pipe        Pipe to get stamps for.  0,1,2 ... (default: 0)\n"
		"  -d delta       Drift time in us to achieve (default: 1000 us) e.g 1000 us = 1.0 ms\n"
		"  -s shift       PLL frequency change fraction (default: 0.1)\n"
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
	pthread_t tid;
	int status;
	double avg_interval;

	INFO("Synctest Version: %s\n", get_version().c_str());

	int delta = 1000;  //  Drift in us to achieve from current time
	double shift = 0.01;
	int pipe = 0;  // Default pipe# 0
	int opt;
	while ((opt = getopt(argc, argv, "p:d:s:v:h")) != -1) {
		switch (opt) {
			case 'p':
				pipe = std::stoi(optarg);
				break;
			case 'd':
				delta = std::stoi(optarg);
				break;
			case 's':
				shift = std::stod(optarg);
				break;
			case 'v':
				set_log_level(optarg);
				break;
			case 'h':
			case '?':
				if (optopt == 'p' || optopt == 'd' || optopt == 's' || optopt == 'v') {
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

	avg_interval = get_vblank_interval(pipe);
	INFO("VBlank interval before starting synchronization: %lf ms\n", avg_interval);


	// synchronize_vsync function is synchronous call and does not
	// output any information. To enhance visibility, a thread is
	// created for logging vblank intervals while synchronization is
	// in progress.
	status = pthread_create(&tid, NULL, background_task, &pipe);
	if (status != 0) {
		ERR("Cannot create thread");
		return 1;
	}

	// Convert delta to milliseconds before calling
	synchronize_vsync((double) delta / 1000.0, pipe, shift);
	// Set flag to 0 to signal the thread to terminate
	thread_continue = 0;
	// Wait for the thread to terminate
	pthread_join(tid, NULL);
	avg_interval = get_vblank_interval(pipe);
	INFO("VBlank interval after synchronization ends: %lf ms\n", avg_interval);

	vsync_lib_uninit();
	return ret;
}

