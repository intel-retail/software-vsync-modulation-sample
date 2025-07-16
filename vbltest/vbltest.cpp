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
double find_avg(uint64_t* va, int sz)
{
	double avg = 0;
	// Return 0 or handle error if there are less than 2 elements
	if (sz < 2) return 0;
	for (int i = 0; i < sz - 1; i++) {
		avg += va[i + 1] - va[i];
	}
	return avg / (sz - 1);
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
void print_vsyncs(char* msg, uint64_t* va, int sz)
{
	char buffer[80];

	if (sz <= 0) {
		INFO("No VBlank timestamps available.\n");
		return;
	}

	unsigned min_offset = va[0] % 1000000;

	INFO("%sVSYNCS\n", msg);
	for (int i = 0; i < sz; i++) {

		uint64_t microseconds_since_epoch = va[i];
		time_t seconds_since_epoch = microseconds_since_epoch / 1000000;

		unsigned offset = microseconds_since_epoch % 1000000;
		if (offset < min_offset)
			min_offset = offset;

		// Convert to local time
		struct tm* local_time = localtime(&seconds_since_epoch);
		// Prepare buffer for formatted date/time

		// Format the local time according to the system's locale
		strftime(buffer, sizeof(buffer), "%x %X", local_time);  // %x for date, %X for time

		// Print formatted date/time and microseconds
		INFO("Received VBlank time stamp [%2d]: %llu -> %s [+%-6u us] \n", i,
			microseconds_since_epoch,
			buffer,
			static_cast<unsigned int>(microseconds_since_epoch % 1000000));
	}

	INFO("First timestamp in the second: +%u us\n", min_offset);
}

/**
 * @brief
 * Print help message
 *
 * @param program_name - Name of the program
 * @return void
 */
void print_help(const char* program_name)
{
	// Using printf for printing help
	printf("Usage: %s [-p pipe] [-c vsync_count] [-v loglevel] [-h]\n"
		"Options:\n"
		"  -p pipe        Pipe to get stamps for.  0,1,2 ... (default: 0)\n"
		"  -c vsync_count Number of vsyncs to get timestamp for (default: 100)\n"
		"  -e device      Device string (default: /dev/dri/card0)\n"
		"  -l loop        Loop mode: 0 = no loop, 1 = loop (default: 0)\n"
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
int main(int argc, char* argv[])
{
	int ret = 0;
	uint64_t* client_vsync;
	double avg;

	printf("Vbltest Version: %s\n", get_version( ).c_str( ));
	int vsync_count = VSYNC_MAX_TIMESTAMPS;  // number of vsyncs to get timestamp for
	std::string device_str = find_first_dri_card();
	std::string log_level = "info";
	int loop_mode = 0;
	int pipe = 0;  // Default pipe# 0
	int opt;
	while ((opt = getopt(argc, argv, "p:c:e:l:v:h")) != -1) {
		switch (opt) {
			case 'p':
				pipe = std::stoi(optarg);
				break;
			case 'c':
				vsync_count = std::stoi(optarg);
				break;
			case 'l':
				loop_mode = std::stoi(optarg);
				break;
			case 'v':
				log_level = optarg;
				set_log_level_str(optarg);
				break;
			case 'e':
				device_str = optarg;
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
	INFO("\tSync Count: %d\n", vsync_count);
	INFO("\tLog Level: %s\n", log_level.c_str());

	print_drm_info(device_str.c_str());

	client_vsync = new uint64_t[vsync_count];
	if (!client_vsync) {
		ERR("Memory allocation failed for client_vsync.\n");
		return 1; // Exit if memory allocation fails
	}

	do {
		if (get_vsync(device_str.c_str(), client_vsync, vsync_count, pipe)) {
			delete[] client_vsync;
			return 1;
		}

		avg = find_avg(&client_vsync[0], vsync_count);

		// Clear the console
		printf("\033[2J\033[H");  // clear screen and move cursor to top
		fflush(stdout);

		print_vsyncs((char*)"", client_vsync, vsync_count);
		INFO("Time average of the vsyncs on the primary system is %.3lf microseconds\n", avg);
	} while(loop_mode); // Keep printing while Ctrl+C is not pressed

	delete[] client_vsync;
	return ret;
}
