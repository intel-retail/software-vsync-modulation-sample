#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <vsyncalter.h>
#include <debug.h>
#include "connection.h"

using namespace std;

connection server, client;

/*******************************************************************************
 * Description
 *	usage - prints the usage of the program
 * Parameters
 *	NONE
 * Return val
 *	void
 ******************************************************************************/
void usage()
{
	PRINT("\nUsage: vsync_test pri/sec [primary's_name_or_ip_addr]\n");
	PRINT("pri = This is the primary system and it needs to share its vsync with others\n");
	PRINT("sec = This is the secondary system. More than one systems can be secondary.\n");
	PRINT("\tIt will synchronize it's vsync to start with primary\n");
	PRINT("primary's_name_or_ip_addr = Only needed for secondary systems. This is the primary\n");
	PRINT("\tsystem's hostname or IP address so that secondary system can communicate with it\n");
}

/*******************************************************************************
 * Description
 *	close_signal - This function closes the server's socket
 * Parameters
 *	int sig - The signal that was received
 * Return val
 *	void
 ******************************************************************************/
void close_signal(int sig)
{
	DBG("Closing server's socket\n");
	server.close_server();
	vsync_lib_uninit();
}

/*******************************************************************************
 * Description
 *	print_vsyncs - This function prints out the last N vsyncs that the system has
 *	received either on its own or from another system. It will only print in DBG
 *	mnode
 * Parameters
 *	char *msg - Any prefix message to be printed.
 *	long *va - The array of vsyncs
 *	int sz - The size of this array
 * Return val
 *	void
 ******************************************************************************/
void print_vsyncs(char *msg, long *va, int sz)
{
	DBG("%s VSYNCS\n", msg);
	for(int i = 0; i < sz; i++) {
		DBG("%ld\n", va[i]);
	}
}

/*******************************************************************************
 * Description
 *	do_msg - This function is used by the server side to get last 10 vsyncs, send
 *	them to the client and receive an ACK from it. Once reveived, it terminates
 *	connection with the client.
 * Parameters
 *	int new_sockfd - The socket on which we need to communicate with the client
 * Return val
 *	int - 0 = SUCCESS, 1 = FAILURE
 ******************************************************************************/
int do_msg(int new_sockfd)
{
	int ret = 0;
	msg m, r;
	long *va = m.get_va();
	int sz = m.get_size();

	if(get_vsync(va, sz)) {
		close(new_sockfd);
		return 1;
	}

	print_vsyncs((char *) "", va, sz);
	do {
		m.add_vsync();
		if(server.send_msg(&m, new_sockfd)) {
			ret = 1;
			break;
		}
		INFO("Sent vsyncs to the secondary system\n");

		if(server.recv_msg(&r, new_sockfd)) {
			ret = 1;
			break;
		}

	} while(r.get_type() != ACK);

	close(new_sockfd);
	return ret;
}

/*******************************************************************************
 * Description
 *	do_primary - This function takes all the actions of the primary system which
 *	are to initialize the server, wait for any clients, dispatch a function to
 *	handle each client and then wait for another client to join until the user
 *	Ctrl+C's out.
 * Parameters
 *	NONE
 * Return val
 *	int - 0 = SUCCESS, 1 = FAILURE
 ******************************************************************************/
int do_primary()
{
	if(server.init_server()) {
		ERR("Failed to init socket connection\n");
		return 1;
	}

	signal(SIGINT, close_signal);
	signal(SIGTERM, close_signal);

	while(1) {
		int new_socket;
		INFO("Waiting for clients\n");
		if(server.accept_client(&new_socket)) {
			return 1;
		}
		INFO("Accepted client\n");

		if(do_msg(new_socket)) {
			return 1;
		}
	}

	return 0;
}

/*******************************************************************************
 * Description
 *	find_avg - This function finds the average of all the vertical syncs that
 *	have been provided by the primary system to the secondary.
 * Parameters
 *	long *va - The array holding all the vertical syncs of the primary system
 *	int sz - The size of this array
 * Return val
 *	long - The average of the vertical syncs
 ******************************************************************************/
long find_avg(long *va, int sz)
{
	int avg = 0;
	for(int i = 0; i < sz - 1; i++) {
		avg += va[i+1] - va[i];
	}
	return avg / ((sz == 1) ? sz : (sz - 1));
}

/*******************************************************************************
 * Description
 *	do_secondary - This function takes all the actions of the secondary system
 *	which are to initialize the client, receive the vsyncs from the server, send
 *	it an ack back, finds its own vsync, calculate the delta between the two
 *	and then synchronize its vsyncs to the primary systems.
 * Parameters
 *	char *server_name_or_ip_addr - The server's hostname or IP address
 * Return val
 *	int - 0 = SUCCESS, 1 = FAILURE
 ******************************************************************************/
int do_secondary(char *server_name_or_ip_addr)
{
	msg m, r;
	int ret = 0;
	long client_vsync, delta, avg;
	long *va;
	int sz;

	if(client.init_client(server_name_or_ip_addr)) {
		return 1;
	}

	do {
		if(client.recv_msg(&m)) {
			ret = 1;
		}

		ret == 0 ? r.ack() : r.nack();
		if(client.send_msg(&r)) {
			ret = 1;
		}
	} while(ret);
	INFO("Received vsyncs from the primary system\n");

	if(get_vsync(&client_vsync, 1)) {
		return 1;
	}

	va = m.get_va();
	sz = m.get_size();

	print_vsyncs((char *) "PRIMARY'S", va, sz);
	print_vsyncs((char *) "SECONDARY'S", &client_vsync, 1);
	delta = client_vsync - va[sz-1];
	avg = find_avg(va, sz);
	INFO("Time average of the vsyncs on the primary system is %ld\n", avg);
	INFO("Time difference between secondary and primary is %ld\n", delta);
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
	if(delta > ONE_VSYNC_PERIOD_IN_MS * 1000 ||
			delta < ONE_VSYNC_PERIOD_IN_MS * 1000) {
		delta %= avg;
		INFO("Time difference between secondary and primary's next vsync is %ld\n", delta);
	}
	//	synchronize_vsync((double) delta / 1000 );
	return ret;
}

/*******************************************************************************
 * Description
 *	main - This is the main function
 * Parameters
 *	int argc - The number of command line arguments
 *	char *argv[] - Each command line argument in an array
 * Return val
 *	int - 0 = SUCCESS, 1 = FAILURE
 ******************************************************************************/
int main(int argc, char *argv[])
{
	int ret = 0;
	if((argc == 2 && strcasecmp(argv[1], "pri")) ||
		(argc == 3 && strcasecmp(argv[1], "sec")) ||
		(argc < 2 || argc > 3)) {
		ERR("Invalid parameters\n");
		usage();
		return 1;
	}

	if(vsync_lib_init()) {
		ERR("Couldn't initialize vsync lib\n");
		return 1;
	}

	if(!strcasecmp(argv[1], "pri")) {
		ret = do_primary();
	} else if(!strcasecmp(argv[1], "sec")) {
		ret = do_secondary(argv[2]);
	}

	vsync_lib_uninit();
	return ret;
}
