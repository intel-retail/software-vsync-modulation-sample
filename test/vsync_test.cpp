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
#include <stdlib.h>
#include "connection.h"
#include "message.h"
#include "version.h"

using namespace std;

connection *server, *client;
int client_done = 0;

/**
* @brief
* Prints the usage of the program
* @param None
* @return void
*/
void usage()
{
	PRINT("Vsynctest Version: %s\n", get_version().c_str());
	PRINT("\nUsage: vsync_test pri/sec [primary's_name_or_ip_addr] [primary's PTP eth addr] [sync after # us]\n");
	PRINT("pri = This is the primary system and it needs to share its vsync with others\n");
	PRINT("sec = This is the secondary system. More than one systems can be secondary.\n");
	PRINT("\tIt will synchronize it's vsync to start with primary\n");
	PRINT("primary's_name_or_ip_addr = Only needed for secondary systems. This is the primary\n");
	PRINT("\tsystem's hostname or IP address so that secondary system can communicate with it\n");
	PRINT("\tNote that if you using PTP, then this is the primary's PTP interface (ex:enp176s0)\n");
	PRINT("primary's_ptp_eth_addr = Only needed for secondary systems. If we are using PTP\n");
	PRINT("protocol, then this is the primary system's PTP interface's ethernet address\n");
	PRINT("(ex:84:47:09:04:eb:0e)\n");
}

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
void print_vsyncs(char *msg, long *va, int sz)
{
	DBG("%s VSYNCS\n", msg);
	for(int i = 0; i < sz; i++) {
		DBG("%ld\n", va[i]);
	}
}

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
* This function is used by the server side to get last 10 vsyncs, send
* them to the client and receive an ACK from it. Once reveived, it terminates
* connection with the client.
* @param new_sockfd - The socket on which we need to communicate with the client
* @return
* - 0 = SUCCESS
* - 1 = FAILURE
*/
int do_msg(int new_sockfd)
{
	int ret = 0;
	msg m, r;
	long *va = m.get_va();

	do {
		memset(&m, 0, sizeof(m));

		if(server->recv_msg(&r, sizeof(r), new_sockfd)) {
			ret = 1;
			break;
		}

		if(get_vsync(va, r.get_vblank_count())) {
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
int do_primary(char *ptp_if)
{
	if(ptp_if) {
		server = new ptp_connection(ptp_if);
	} else {
		server = new connection();
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

		if(do_msg(new_socket)) {
			return 1;
		}
	}

	return 0;
}

/**
* @brief
* This function takes all the actions of the secondary system
* which are to initialize the client, receive the vsyncs from the server, send
* it an ack back, finds its own vsync, calculate the delta between the two
* and then synchronize its vsyncs to the primary systems.
* @param *server_name_or_ip_addr - The server's hostname or IP address. In case,
* the user wants us to communicate over PTP, then this holds the server's PTP
* interface
* @param *ptp_eth_address - In case, the user wants us to communicate over PTP,
* this holds the server's PTP interfaces' ethernet address.
* @param synchronize - Whether to synchronize or not.
* - 0 = do not synchronize, just exit after getting vblanks
* -	1 = Synchronize once and exit
* @param timestamps - How many timestamps to collect for primary and secondary
* @return
* - 0 = SUCCESS
* - 1 = FAILURE
*/
int do_secondary(
	char *server_name_or_ip_addr,
	char *ptp_eth_address,
	int synchronize,
	int timestamps)
{
	msg m, r;
	int ret = 0;
	long client_vsync[MAX_TIMESTAMPS], delta, avg;
	long *va;
	static int counter = 0;

	if(ptp_eth_address) {
		client = new ptp_connection(server_name_or_ip_addr, ptp_eth_address);
	} else {
		client = new connection();
	}

	if(client->init_client(server_name_or_ip_addr)) {
		delete client;
		client = NULL;
		return 1;
	}

	if(timestamps > MAX_TIMESTAMPS) {
		ERR("Number of vsync timestamps to collect can't be greater than or equal to %d", MAX_TIMESTAMPS);
		delete client;
		client = NULL;
		return 1;
	}

	do {
		r.ack();
		r.set_vblank_count(timestamps);
		if(client->send_msg(&r, sizeof(r))) {
			ret = 1;
		}

		if(client->recv_msg(&m, sizeof(m))) {
			ret = 1;
		}
		m.compare_time();

	} while(ret);
	DBG("Received vsyncs from the primary system\n");

	if(get_vsync(client_vsync, timestamps)) {
		delete client;
		client = NULL;
		return 1;
	}

	va = m.get_va();

	print_vsyncs((char *) "PRIMARY'S", va, timestamps);
	print_vsyncs((char *) "SECONDARY'S", client_vsync, timestamps);
	delta = client_vsync[0] - va[timestamps-1];
	avg = find_avg(va, timestamps);
	DBG("Time average of the vsyncs on the primary system is %ld us\n", avg);
	avg = find_avg(client_vsync, timestamps);
	DBG("Time average of the vsyncs on the secondary system is %ld us\n", avg);
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
	if(delta > avg || delta < avg) {
		delta %= avg;
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
	if(delta > avg/2) {
		delta -= avg;
	}
	INFO("Time difference between secondary and primary's next vsync is %ld us\n", delta);

	if(synchronize) {
		if(abs(delta) > synchronize) {
			synchronize_vsync((double) delta / 1000 );
			INFO("Synchronizing after %d seconds\n", counter);
			counter = 0;
		}
		counter++;
	}

	if(client) {
		client->close_client();
		delete client;
		client = NULL;
	}

	return ret;
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
	int ret = 0, us_synchronize, timestamps = MAX_TIMESTAMPS;
	if(!(((argc == 2 || argc == 3) && !strncasecmp(argv[1], "pri", 3)) ||
		((argc == 3  || argc == 4 || argc == 5) && !strncasecmp(argv[1], "sec", 3))) ||
		(argc < 2 || argc > 5)) {
		ERR("Invalid parameters\n");
		usage();
		return 1;
	}

	if(vsync_lib_init()) {
		ERR("Couldn't initialize vsync lib\n");
		return 1;
	}

	if(!strncasecmp(argv[1], "pri", 3)) {
		ret = do_primary(argc == 3 ? argv[2] : NULL);
	} else if(!strncasecmp(argv[1], "sec", 3)) {

		signal(SIGINT, client_close_signal);
		signal(SIGTERM, client_close_signal);

		switch(argc) {
			case 3:
				ret = do_secondary(argv[2], NULL, 1, MAX_TIMESTAMPS);
			break;
			case 4:
				if(strchr(argv[3], ':')) {
					ret = do_secondary(argv[2], argv[3], 1, MAX_TIMESTAMPS);
				} else {
					ret = do_secondary(argv[2], NULL, atoi(argv[3]), MAX_TIMESTAMPS);
				}
			break;
			case 5:
				us_synchronize = atoi(argv[4]);

				do {
					ret = do_secondary(argv[2], argv[3], us_synchronize, timestamps);

					if(us_synchronize != 0 && us_synchronize != 1) {
						timestamps = 2;
						sleep(1);
					}
				} while(us_synchronize != 0 && us_synchronize != 1 && !client_done && !ret);
			break;
		}
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
	vsync_lib_uninit();
	return ret;
}
