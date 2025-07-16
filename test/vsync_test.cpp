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
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>
#include <vsyncalter.h>
#include <debug.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <regex>
#include <getopt.h>
#include "connection.h"
#include "message.h"
#include "version.h"

using namespace std;

connection *server, *client;
int client_done = 0;
int thread_continue = 1;
struct timespec g_last;

#define MAX_DEVICE_NAME_LENGTH 64
char g_devicestr[MAX_DEVICE_NAME_LENGTH];

/**
* @brief
* This function closes the server's socket
* @param sig - The signal that was received
* @return void
*/
void server_close_signal(int sig)
{
	DBG("Closing server's socket\n");
	server->close_server();
	delete server;
	server = NULL;
	vsync_lib_uninit();
	exit(1);
}

/**
* @brief
* This function closes the client's socket
* @param sig - The signal that was received
* @return void
*/
void client_close_signal(int sig)
{
	DBG("Closing client's socket\n");

	// Inform vsync lib about Ctrl+C signal
	shutdown_lib();

	client_done = 1;
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
void print_vsyncs(char *msg, uint64_t *va, int sz)
{
	char buffer[80];

	DBG("%s VSYNCS\n", msg);
	for(int i = 0; i < sz; i++) {
		uint64_t microseconds_since_epoch = va[i];
		time_t seconds_since_epoch = microseconds_since_epoch / 1000000;

		// Convert to local time
		struct tm *local_time = localtime(&seconds_since_epoch);
		// Prepare buffer for formatted date/time

		// Format the local time according to the system's locale
		strftime(buffer, sizeof(buffer), "%x %X", local_time);  // %x for date, %X for time

		// Print formatted date/time and microseconds
		DBG("Received VBlank time stamp [%2d]: %llu -> %s [+%-6u us] \n", i,
			microseconds_since_epoch,
			buffer,
			static_cast<unsigned int>(microseconds_since_epoch % 1000000));
	}
}

#define LOG_PREFIX "sync_pipe_"
#define LOG_SUFFIX ".csv"
char log_filename[128];  // Holds the log file name with date and time
struct timeval log_start_time;
// Call this once at program start
void init_log_file_name(int pipe) {
	time_t now = time(NULL);
	struct tm *tm_info = localtime(&now);
	char datetime[32];

	// Format: YYYY-MM-DD_HH-MM-SS
	strftime(datetime, sizeof(datetime), "%Y%m%d_%H%M%S", tm_info);

	// Generate filename: synchronization_pipe_<pipe>_<datetime>.log
	snprintf(log_filename, sizeof(log_filename), "%s%d_%s%s",
			LOG_PREFIX, pipe, datetime, LOG_SUFFIX);
	gettimeofday(&log_start_time, NULL);  // Save start time
}

// log_to_file with printf-style formatting
void log_to_file(bool comment, const char *fmt, ...) {
	FILE *fp = fopen(log_filename, "a");
	if (!fp) {
		perror("Failed to open log file");
		return;
	}

	struct timeval now;
	gettimeofday(&now, NULL);

	long delta_sec = now.tv_sec - log_start_time.tv_sec;
	long delta_usec = now.tv_usec - log_start_time.tv_usec;
	if (delta_usec < 0) {
		delta_sec--;
		delta_usec += 1000000;
	}

	double delta = delta_sec + (delta_usec / 1e6);

	// Format the message
	char message[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(message, sizeof(message), fmt, args);
	va_end(args);

	// Log format: [+1.234s] Your message
	fprintf(fp, "%s[+%.3fs] %s\n", comment ? "#": "", delta, message);
	fclose(fp);
}

/**
* @brief
* This function finds the average of all the vertical syncs that
* have been provided by the primary system to the secondary.
* @param *va - The array holding all the vertical syncs of the primary system
* @param sz - The size of this array
* @return The average of the vertical syncs
*/
long find_avg(uint64_t *va, int sz)
{
	int avg = 0;
	for(int i = 0; i < sz - 1; i++) {
		avg += va[i+1] - va[i];
	}
	return avg / ((sz == 1) ? sz : (sz - 1));
}

/**
* @brief
* This function is used by the server side to get last 10 vsyncs, send
* them to the client and receive an ACK from it. Once reveived, it terminates
* connection with the client.
* @param new_sockfd - The socket on which we need to communicate with the client
* @return
* - 0 = SUCCESS
* - 1 = FAILURE
*/
int do_msg(int new_sockfd, int pipe)
{
	int ret = 0;
	msg m, r;
	uint64_t *va = m.get_va();

	do {
		memset(&m, 0, sizeof(m));

		if(server->recv_msg(&r, sizeof(r), new_sockfd)) {
			ret = 1;
			break;
		}

		if(get_vsync(g_devicestr, va, r.get_vblank_count(), pipe)) {
			close(new_sockfd);
			return 1;
		}

		print_vsyncs((char *) "", va, r.get_vblank_count());
		m.add_vsync();
		m.add_time();

		if(server->send_msg(&m, sizeof(m), new_sockfd)) {
			ret = 1;
			break;
		}
		INFO("Sent vsyncs to the secondary system\n");

	} while(r.get_type() != ACK);

	close(new_sockfd);
	return ret;
}

bool isValidIPv4(const std::string &ip)
{
	try {
			static const std::regex ipv4_pattern(
				R"((^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(25[0-5]
					|2[0-4][0-9]|[01]?[0-9][0-9]?)$))");
			return std::regex_match(ip, ipv4_pattern);
	}
	catch (const std::regex_error &e) {
			// Log or handle the regex error
			return false;
	}
	catch (...) {
			// Handle other unforeseen exceptions
			return false;
	}
}

/**
* @brief
* This function takes all the actions of the primary system which
* are to initialize the server, wait for any clients, dispatch a function to
* handle each client and then wait for another client to join until the user
* Ctrl+C's out.
* @param *ptp_if - This is the PTP interface provided by the user on command
* line. It can also be NULL, in which case they would rather have us
* communicate via TCP.
* @return
* - 0 = SUCCESS
* - 1 = FAILURE
*/
int do_primary(const char *ptp_if, int pipe)
{
	if(!isValidIPv4(std::string(ptp_if))) {
		server = new ptp_connection(ptp_if);
	} else {
		server = new connection(ptp_if);
	}

	if(server->init_server()) {
		ERR("Failed to init socket connection\n");
		return 1;
	}

	signal(SIGINT, server_close_signal);
	signal(SIGTERM, server_close_signal);

	while(1) {
		int new_socket;
		INFO("Waiting for clients\n");
		if(server->accept_client(&new_socket)) {
			return 1;
		}

		if(do_msg(new_socket, pipe)) {
			return 1;
		}
	}

	return 0;
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
		INFO("\t[Elapsed Time: %.2lf sec] VBlank interval on pipe %d is %.4lf ms\n", elapsed_time, pipe, avg_interval);
	}

	avg_interval = get_vblank_interval(g_devicestr, pipe, 30);
	INFO("VBlank interval after synchronization ends: %.4lf ms\n", avg_interval);
	return NULL;
}

// Function to convert timespec structure to milliseconds
long long timespec_to_ms(struct timespec *ts) {
	return (ts->tv_sec * 1000LL) + (ts->tv_nsec / 1000000LL);
}

int isNotZero(double value) {
	const double epsilon = 1e-9;   // Small threshold to check if delta is zero
	return fabs(value) >= epsilon;
}

/**
* @brief
* This function takes all the actions of the secondary system
* which are to initialize the client, receive the vsyncs from the server, send
* it an ack back, finds its own vsync, calculate the delta between the two
* and then synchronize its vsyncs to the primary systems.
* @param server_name_or_ip_addr - The server's hostname or IP address. In case,
* the user wants us to communicate over PTP, then this holds the server's PTP
* interface
* @param ptp_eth_address - In case, the user wants us to communicate over PTP,
* 				this holds the server's PTP interfaces' ethernet address.
* @param pipe - Pipe id
* @param sync_threshold_us - Whether to synchronize or not.
*       -  0 = do not synchronize, just exit after getting vblanks
*       -  1 = Synchronize once and exit
* @param timestamps - How many timestamps to collect for primary and secondary
* @param shift - Shift value to use
* @param time_period - Time period in seconds during which learning rate will be applied
* @param learning_rate - Learning rate to apply during synchronization
* @param shift2 - Shift value to use for extended calculation (step by step)
* @param overshoot_ratio - Overshoot ratio to apply during synchronization
* @param step_threshold - step_threshold  Delta threshold in microseconds to trigger stepping mode
* @param wait_between_steps - Wait in milliseconds between steps
* @return
* - 0 = SUCCESS
* - 1 = FAILURE
*/
int do_secondary(
	const char *server_ip,
	const char *eth_addr,
	int pipe,
	int sync_threshold_us,
	int timestamps,
	double shift,
	int time_period,
	double learning_rate,
	double shift2,
	double overshoot_ratio,
	int step_threshold,
	int wait_between_steps)
{
    msg m, r;
    int ret = 0;
    uint64_t client_vsync[VSYNC_MAX_TIMESTAMPS], *primary_vsync;
    long delta, avg_primary, avg_secondary;
    pthread_t tid;
    int status;
    struct timespec now;
    int64_t duration;
    static u_int32_t success_iter = 0, sync_count = 0, out_of_sync = 0;
	const int ns_in_ms =  1000000;

	client = eth_addr ? new ptp_connection(server_ip, eth_addr)
						: new connection(server_ip);

	if(client->init_client(server_ip)) {
		goto cleanup_fail;
	}

	if(timestamps > VSYNC_MAX_TIMESTAMPS) {
		ERR("Too many timestamps (max %d)", VSYNC_MAX_TIMESTAMPS);
		goto cleanup_fail;
	}

	do {
		r.ack();
		r.set_vblank_count(timestamps);
		ret = client->send_msg(&r, sizeof(r)) || client->recv_msg(&m, sizeof(m));
	} while(ret);

	DBG("Received vsyncs from the primary system\n");

	if(get_vsync(g_devicestr, client_vsync, timestamps, pipe)) {
		goto cleanup_fail;
	}

	primary_vsync = m.get_va();

	// Check if the clocks on both systems are in sync.  Give margin of 35 ms
	if (llabs(client_vsync[0] - primary_vsync[timestamps-1]) > 35 * ns_in_ms) {
		ERR("Primary and secondary clocks are not synchronized.");
		goto cleanup;
	}

	print_vsyncs((char *) "PRIMARY'S", primary_vsync, timestamps);
	print_vsyncs((char *) "SECONDARY'S", client_vsync, timestamps);

	delta = client_vsync[0] - primary_vsync[timestamps-1];
	avg_primary = find_avg(primary_vsync, timestamps);
	avg_secondary = find_avg(client_vsync, timestamps);

	DBG("Time average of the vsyncs on the primary system is %ld us\n", avg_primary);
	DBG("Time average of the vsyncs on the secondary system is %ld us\n", avg_secondary);
	DBG("Time difference between secondary and primary is %ld us\n", delta);
	/*
	 * If the primary is ahead or behind the secondary by more than a vsync,
	 * we can just adjust the secondary's vsync to what we think the primary's
	 * next vsync would be happening at. We do this by calculating the average
	 * of its last N vsyncs that it has provided to us and assuming that its
	 * next vsync would be happening at this cadence. Then we modulo the delta
	 * with this average to give us a time difference between it's nearest
	 * vsync to the secondary's. As long as we adjust the secondary's vsync
	 * to this value, it would basically mean that the primary and secondary
	 * system's vsyncs are firing at the same time.
	 */
	if(delta > avg_secondary || delta < avg_secondary) {
		delta %= avg_secondary;
	}

	/*
	 * If the time difference between primary and secondary is larger than the
	 * mid point of secondary's vsync time period, then it makes sense to sync
	 * with the next vsync of the primary. For example, say primary's vsyncs
	 * are happening at a regular cadence of 0, 16.66, 33.33 ms while
	 * secondary's vsyncs are happening at a regular candence of 10, 26.66,
	 * 43.33 ms, then the time difference between them is exactly 10 ms. If
	 * we were to make the secondary faster for each iteration so that it
	 * walks back those 10 ms and gets in sync with the primary, it would have
	 * taken us about 600 iterations of vsyncs = 10 seconds. However, if were
	 * to make the secondary slower for each iteration so that it walks forward
	 * then since it is only 16.66 - 10 = 6.66 ms away from the next vsync of
	 * the primary, it would take only 400 iterations of vsyncs = 6.66 seconds
	 * to get in sync. Therefore, the general rule is that we should make
	 * secondary's vsyncs walk back only if it the delta is less than half of
	 * secondary's vsync time period, otherwise, we should walk forward.
	 */
	if(delta > avg_secondary/2) {
		delta -= avg_secondary;
	}

	clock_gettime(CLOCK_MONOTONIC, &now);

	// Calculate the duration in milliseconds
	duration = timespec_to_ms(&now) - timespec_to_ms(&g_last);

	DBG("Time average of the vsyncs: Primary = %.3f ms, Secondary = %.3f ms, Delta = %ld us\n", avg_primary/1000.0, avg_secondary/1000.0, delta);
	INFO("Delta: %4ld us [%.3f sec since last sync]\n", delta, duration/1000.0);


	if(sync_threshold_us && abs(delta) > sync_threshold_us) {

		out_of_sync++;
		// Carry out synchronization only after two iterations of out_of_sync
		// Or if it's the beginning and we have not synchronized yet
		if (out_of_sync < 2 && sync_count > 1) {
			goto cleanup;
		}

		out_of_sync = 0; // Reset out_of_sync count

		clock_gettime(CLOCK_MONOTONIC, &now);
		// Calculate the duration in milliseconds
		duration = timespec_to_ms(&now) - timespec_to_ms(&g_last);
		g_last = now;
		// Reverse delta direction to achieve desired drift by converting
		// microseconds to milliseconds and negating.
		double delta_ms = (delta * -1.0) / 1000.0;
		double current_freq = get_pll_clock(pipe);

		log_to_file(false, ",%7.3f,%3ld,%.3f", duration/1000.0, delta, current_freq);

		INFO("Synchronizing after %.3f seconds.\n", duration/1000.0);

		thread_continue = 1;
		// synchronize_vsync function is synchronous call and does not
		// output any information. To enhance visibility, a thread is
		// created for logging vblank intervals while synchronization is
		// in progress.
		status = pthread_create(&tid, NULL, background_task, &pipe);
		if (status != 0) {
			ERR("Thread creation failed");
			ret = 1;
			goto cleanup;
		}

		// We don't want to apply overshoot ratio along with adaptive learning.
		// Apply overshoot ratio if either learning is disabled or we are already
		// beyond time period when learning is not applied
		if(!isNotZero(learning_rate) || duration > ((int64_t)time_period * 1000)) {
			delta_ms *= (1.0 + overshoot_ratio); // Apply overshoot ratio
		}

		synchronize_vsync(delta_ms, pipe, shift, shift2, step_threshold, wait_between_steps, true, true);
		thread_continue = 0; // Set flag to 0 to signal the thread to terminate
		pthread_join(tid, NULL); // Wait for the thread to terminate

		// Adaptive Learning:
		// If learning rate is provided and atleast one sync was trigger before (allow for atleast one sync)
		// and had atleast one successful check and the duration since the last synchronization is less
		// than the specified time period, adjust the clocks using the learning rate.
		// The check for sync_count > 1 ensures that we only start apply learning after the first synchronization,
		// allowing the system to stabilize.
		if (isNotZero(learning_rate) && sync_count > 1
				&& success_iter > 0
				&& duration < ((int64_t)time_period * 1000)) {

			usleep(100*1000); // Give some time for the clocks to adjust from earlier sync
			INFO("Adaptive Learning after %.3f secs (%d iterations)\n", duration / 1000.0, success_iter);

			// delta_ms is used to determine the adjustment direction (increase or decrease).
			// The learning_rate is used as the shift value.  The 'false' parameter indicates
			// not to reset values after writing and to return immediately.
			synchronize_vsync((double) delta_ms, pipe, learning_rate, 0.0, step_threshold, wait_between_steps, false, true);
		}

		clock_gettime(CLOCK_MONOTONIC, &g_last);
		success_iter = 0;  // Reset iteration count
		sync_count++;

	}
	else {
		out_of_sync = 0; // Reset out_of_sync count
		success_iter++;
	}

cleanup:
	if (client) {
		client->close_client();
		delete client;
		client = NULL;
	}
	return ret;

cleanup_fail:
	ret = 1;
	goto cleanup;
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
	printf("Usage: %s [-m mode] [-i interface] [-c mac_address] [-d delta] [-p pipe] [-s shift] [-v loglevel] ... [-h]\n"
		"Options:\n"
		"  -m mode            Mode of operation: pri, sec (default: pri)\n"
		"  -i interface       Network interface to listen on (primary) or connect to (secondary) (default: 127.0.0.1)\n"
		"  -c mac_address     MAC address of the network interface to connect to. Applicable to ethernet interface mode only.\n"
		"  -d delta           Drift time in microseconds to allow before pll reprogramming - in microseconds (default: 100 us)\n"
		"  -p pipe            Pipe to get stamps for.  4 (all) or 0,1,2 ... (default: 0)\n"
		"  -s shift           PLL frequency change fraction (default: 0.01)\n"
		"  -x shift2          PLL frequency change fraction for large drift (default: 0.0; Disabled)\n"
		"  -f frequency       PLL clock value to set at start (default -> Do Not Set : 0.0) \n"
		"  -e device          Device string (default: /dev/dri/card0)\n"
		"  -v loglevel        Log level: error, warning, info, debug or trace (default: info)\n"
		"  -k time_period     Time period in seconds during which learning rate will be applied.  (default: 240 sec)\n"
		"  -l learning_rate   Learning rate for convergence. Secondary mode only. e.g 0.00001 (default: 0.0  Disabled) \n"
		"  -o overshoot_ratio Allow the clock to go beyond zero alignment by a ratio of the delta (value between 0 and 1). \n"
		"                        For example, with -o 0.5 and delta=500, the target offset becomes -250 us in the apposite direction (default: 0.0)\n"
		"  -t step_threshold  Delta threshold in microseconds to trigger stepping mode (default: 1000 us)\n"
		"  -w step_wait       Wait in milliseconds between steps (default: 50 ms) \n"
		"  -n                 Use DP M & N Path. (default: no)\n"
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
	std::string modeStr = "pri";
	std::string log_level = "info";
	std::string interface_or_ip = "127.0.0.1";  // Default to localhost
	std::string mac_address = "";
	std::string device_str = find_first_dri_card();
	int timestamps = VSYNC_MAX_TIMESTAMPS;
	int pipe = 0;  // Default pipe# 0
	int delta = 100;
	double frequency = 0.0;
	double shift = 0.01, shift2=0.0;
	double learning_rate = 0.0001;
	double overshoot_ratio = 0.0; // Default overshoot ratio
	int time_period = 480, step_threshold = VSYNC_TIME_DELTA_FOR_STEP, wait_between_steps = VSYNC_DEFAULT_WAIT_IN_MS;
	bool m_n = false;
	static struct option long_options[] = {
		{"mn", no_argument, NULL, 'n'},
		{0, 0, 0, 0}
	};
	int opt, option_index = 0; // getopt_long stores the option index here
	while ((opt = getopt_long(argc, argv, "m:i:c:p:d:s:x:f:o:e:k:l:n:t:w:v:h", long_options, &option_index)) != -1) {
		switch (opt) {
			case 'm':
				modeStr = optarg;
				break;
			case 'i':
				interface_or_ip = optarg;
				break;
			case 'c':
				mac_address = optarg;
				break;
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
			case 'k':
				time_period = std::stoi(optarg);
				break;
			case 'l':
				learning_rate = std::stod(optarg);
				break;
			case 'n':
				m_n = true;
				break;
			case 'f':
				frequency = std::stod(optarg);
				break;
			case 'o':
				overshoot_ratio = std::stod(optarg);
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
			case 'e':
				device_str=optarg;
				break;
			case 'h':
				print_help(argv[0]);
				exit(EXIT_SUCCESS);
			case '?':
				print_help(argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	if (modeStr == "pri") {
		set_log_mode("[ PRIMARY ]");
	} else if (modeStr == "sec") {
		set_log_mode("[SECONDARY]");
	}

	// Print configurations
	INFO("Configuration:\n");
	INFO("\tMode: %s\n", modeStr.c_str());
	INFO("\tInterface or IP: %s\n", interface_or_ip.length() > 0 ? interface_or_ip.c_str() : "N/A");
	INFO("\tSec Mac Address: %s\n", mac_address.length() > 0 ? mac_address.c_str() : "N/A");
	INFO("\tDelta: %d us\n", delta);
	INFO("\tShift: %.3lf\n", shift);
	INFO("\tShift2: %.3lf\n", shift2);
	INFO("\tFrequency: %lf\n", frequency);
	INFO("\tOvershoot Ratio: %lf\n", overshoot_ratio);
	INFO("\tPipe ID: %d\n", pipe);
	INFO("\tDevice: %s\n", device_str.c_str());
	INFO("\tstep_threshold: %d us\n", step_threshold);
	INFO("\twait_between_steps: %d ms\n", wait_between_steps);
	if (isNotZero(learning_rate)) {
		INFO("\tLearning rate: %lf\n", learning_rate);
		INFO("\ttime_period: %d sec\n", time_period);
	}
	INFO("\tLog Level: %s\n", log_level.c_str());

	memset(g_devicestr,0, MAX_DEVICE_NAME_LENGTH);
	// Copy until src string size or max size - 1.
	strncpy(g_devicestr, device_str.c_str(), MAX_DEVICE_NAME_LENGTH - 1);

	u_int64_t va[VSYNC_MAX_TIMESTAMPS];
	// Get only one timestamp to check if kernel patch was applied.
	if (get_vsync(g_devicestr, va, 1, pipe) == -1) {
		ERR("Failed to get vsync\n");
		return 1;
	}

	//Check if Kernel real time clock patch is applied.
	struct timespec ts;
	if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
		int64_t current_time_in_microseconds = ((unsigned long long)ts.tv_sec * 1000000ULL) + ((unsigned long long)ts.tv_nsec / 1000);
		int64_t difference = current_time_in_microseconds - va[0];
		// Check if kernel patch applied. Get system real time and compare it with vsync time. The
		// difference shouldn't be more than few milliseconds. Giving 100 ms margin.
		const int diff_in_micro=100000; // time in microseconds
		if (labs(difference) >  diff_in_micro) {
			ERR("Kernel patch not applied. Exiting\n");
			return 1;
		}
	} else {
		// Handle error in clock_gettime
		ERR("Failed to get the current time");
		return 1;
	}

	if(!modeStr.compare("pri")) {
		ret = do_primary(interface_or_ip.length() > 0 ? interface_or_ip.c_str() : NULL, pipe);
	} else if(!modeStr.compare("sec")) {
		signal(SIGINT, client_close_signal);
		signal(SIGTERM, client_close_signal);
		clock_gettime(CLOCK_MONOTONIC, &g_last);
		// lib initialization only for secondary mode.
		if(vsync_lib_init(g_devicestr, m_n)) {
			ERR("Failed to initialize vsync library with device: %s\n", g_devicestr);
			return 1;
		}

		char name[32];
		if(!get_phy_name(pipe, name, sizeof(name))) {
			ERR("Failed to get PHY name for pipe %d\n", pipe);
			return 1;
		}

		init_log_file_name(pipe);
		log_to_file(true, "Secondary mode starting");
		log_to_file(true,
						"Configuration:\n"
						"#\tPHY Type: %s\n"
						"#\tPipe ID: %d\n"
						"#\tDelta: %d us\n"
						"#\tShift: %.3lf\n"
						"#\tShift2: %.3lf\n"
						"#\tDevice: %s\n"
						"#\tLearning rate: %lf\n"
						"#\ttime_period: %d sec",
						name,
						pipe,
						delta,
						shift,
						shift2,
						device_str.c_str(),
						learning_rate,
						time_period
					);

		log_to_file(true, "\n#[Time],duration from last sync (sec),drift delta (us),PLL Frequency");
		if (isNotZero(frequency)) {
			INFO("Setting PLL clock value to %lf\n", frequency);
			set_pll_clock(frequency, pipe, shift, wait_between_steps);
		}
		// Keep doing synchronization until the user Ctrl+C's out
		do {
				ret = do_secondary(interface_or_ip.c_str(),
					mac_address.length() > 0 ? mac_address.c_str() : NULL,
					pipe, delta, timestamps, shift, time_period, learning_rate,
					shift2, overshoot_ratio, step_threshold, wait_between_steps);

				timestamps = 2;
				sleep(1);
		} while(!client_done && !ret);

		vsync_lib_uninit();
	}

	if(client) {
		client->close_client();
		delete client;
		client = NULL;
	}
	if(server) {
		delete server;
		server = NULL;
	}
	return ret;
}
