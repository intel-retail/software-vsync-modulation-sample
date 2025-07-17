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
#include <getopt.h>
#include "version.h"

using namespace std;
volatile int thread_continue = 1;

#define MAX_DEVICE_NAME_LENGTH 64
char g_devicestr[MAX_DEVICE_NAME_LENGTH];

/**
* @brief
* This function informs vsynclib about termination.
* @param sig - The signal that was received
* @return void
*/
void terminate_signal(int sig)
{
	thread_continue = 0;

	// Inform vsync lib about Ctrl+C signal
	shutdown_lib();
	INFO("Terminating due to Ctrl+C...\n");
}

/**
 * @brief
 * This function is the background thread task to call print_vblank_interval
 *
 * @param arg
 * @return void*
 */
void* background_task(void* arg)
{
	const int WAIT_TIME_IN_MICRO = 1000 * 1000;  // 1 sec
	double avg_interval;
	int pipe = *(int*)arg;

	INFO("VBlank interval during synchronization ->\n");
	struct timespec start_time, current_time;
	clock_gettime(CLOCK_MONOTONIC, &start_time);

	// Thread runs this loop until it is canceled
	while (thread_continue) {
		usleep(WAIT_TIME_IN_MICRO);
		// Calculate elapsed time
		clock_gettime(CLOCK_MONOTONIC, &current_time);
		double elapsed_time = (current_time.tv_sec - start_time.tv_sec) +
			(current_time.tv_nsec - start_time.tv_nsec) / 1e9;

		avg_interval = get_vblank_interval(g_devicestr, pipe, 30);
		INFO("\t[Elapsed Time: %.2lf s] Time average of the vsyncs on pipe %d is %.4lf ms\n", elapsed_time, pipe, avg_interval);
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
	// Using printf for printing help
	printf("Usage: %s [-p pipe] [-d delta] [-s shift] [-v loglevel] [-h]\n"
		"Options:\n"
		"  -p pipe            Pipe to get stamps for.  0,1,2 ... (default: 0)\n"
		"  -d delta           Drift time in us to achieve (default: 1000 us) e.g 1000 us = 1.0 ms\n"
		"  -s shift           PLL frequency change fraction (default: 0.01)\n"
		"  -x shift2          PLL frequency change fraction for large drift (default: 0.0; Disabled)\n"
		"  -e device          Device string (default: /dev/dri/card0)\n"
		"  -f frequency       Clock value to directly set (default -> Do not set : 0.0) \n"
		"  -v loglevel        Log level: error, warning, info, debug or trace (default: info)\n"
		"  -t step_threshold  Delta threshold in microseconds to trigger stepping mode (default: 1000 us)\n"
		"  -w step_wait       Wait in milliseconds between steps (default: 50 ms) \n"
		"  --no-reset         Do no reset to original values. Keep modified PLL frequency and exit (default: reset)\n"
		"  --no-commit        Do no commit changes.  Just print (default: commit)\n"
		"  -m                 Use DP M & N Path. (default: no)\n"
		"  -h                 Display this help message\n",
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

	printf("Synctest Version: %s\n", get_version().c_str());

	std::string device_str = find_first_dri_card();
	std::string log_level = "info";
	int delta = 1000;  //  Drift in us to achieve from current time
	double shift = 0.01, shift2 = 0.0;
	int pipe = 0;  // Default pipe# 0
	bool reset = true;
	bool commit = true;
	double frequency = 0.0;
	bool m_n = false;
	int step_threshold = VSYNC_TIME_DELTA_FOR_STEP, wait_between_steps = VSYNC_DEFAULT_WAIT_IN_MS;
	static struct option long_options[] = {
		{"no-reset", no_argument, NULL, 'r'},
		{"no-commit", no_argument, NULL, 'c'},
		{"mn", no_argument, NULL, 'm'},
		{0, 0, 0, 0}
	};
	int opt, option_index = 0; // getopt_long stores the option index here
	while ((opt = getopt_long(argc, argv, "p:d:s:x:e:n:r:f:m:t:w:v:h", long_options, &option_index)) != -1) {
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
			case 'x':
				shift2 = std::stod(optarg);
				break;
			case 'e':
				device_str=optarg;
				break;
			case 'r':
				reset = false;
				break;
			case 'c':
				commit = false;
				break;
			case 'm':
				m_n = true;
				break;
			case 't':
				step_threshold = std::stoi(optarg);
				break;
			case 'w':
				wait_between_steps = std::stoi(optarg);
				break;
			case 'v':
				log_level = optarg;
				set_log_level_str(optarg);
				break;
			case 'f':
				frequency = std::stod(optarg);
				break;
			case 'h':
				print_help(argv[0]);
				exit(EXIT_SUCCESS);
			case '?':
				print_help(argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	// Print configurations
	INFO("Configuration:\n");
	INFO("\tPipe ID: %d\n", pipe);
	INFO("\tDevice: %s\n", device_str.c_str());
	INFO("\tDelta: %d microseconds\n", delta);
	INFO("\tShift: %lf\n", shift);
	INFO("\tFrequency to set: %lf\n", frequency);
	INFO("\tLog Level: %s\n", log_level.c_str());
	INFO("\tstep_threshold: %d us\n", step_threshold);
	INFO("\twait_between_steps: %d ms\n", wait_between_steps);

	memset(g_devicestr,0, MAX_DEVICE_NAME_LENGTH);
	// Copy until src string size or max size - 1.
	strncpy(g_devicestr, device_str.c_str(), MAX_DEVICE_NAME_LENGTH - 1);

	if(vsync_lib_init(device_str.c_str(), m_n)) {
		ERR("Failed to initialize vsync library with device: %s\n", device_str.c_str());
		return 1;
	}

	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = terminate_signal;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, NULL);
	sigaction(SIGTERM, &sigIntHandler, NULL);

	if (commit) {
		// synchronize_vsync function is synchronous call and does not
		// output any information. To enhance visibility, a thread is
		// created for logging vblank intervals while synchronization is
		// in progress.
		status = pthread_create(&tid, NULL, background_task, &pipe);
		if (status != 0) {
			ERR("Cannot create thread");
			return 1;
		}
	}

	// If frequency is set, directly set the PLL clock
	if (frequency > 0.0) {
		// shift is used internally to create steps if delta between
		// current and desired frequency is large
		set_pll_clock(frequency, pipe, shift, VSYNC_DEFAULT_WAIT_IN_MS);
	} else {
		// Convert delta to milliseconds before calling
		synchronize_vsync((double) delta / 1000.0, pipe, shift, shift2, step_threshold, wait_between_steps, reset, commit);
	}

	// Set flag to 0 to signal the thread to terminate
	thread_continue = 0;

	if (commit) {
		// Wait for the thread to terminate
		pthread_join(tid, NULL);
		avg_interval = get_vblank_interval(g_devicestr, pipe, VSYNC_MAX_TIMESTAMPS);
		INFO("VBlank interval after synchronization ends: %lf ms\n", avg_interval);
	}

	vsync_lib_uninit();
	return ret;
}

