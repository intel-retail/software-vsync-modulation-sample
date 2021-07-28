#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <vsyncalter.h>
#include <debug.h>
#include "connection.h"

using namespace std;

connection server, client;

void usage()
{
	PRINT("\nUsage: vsync_test pri/sec [primary's_name_or_ip_addr]\n");
	PRINT("pri = This is the primary system and it needs to share its vsync with others\n");
	PRINT("sec = This is the secondary system. More than one systems can be secondary.\n");
	PRINT("\tIt will synchronize it's vsync to start with primary\n");
	PRINT("primary's_name_or_ip_addr = Only needed for secondary systems. This is the primary\n");
	PRINT("\tsystem's hostname or IP address so that secondary system can communicate with it\n");
}

void close_signal(int sig)
{
	INFO("Closing server's socket\n");
	server.close_server();
}

void print_vsyncs(char *msg, long *va, int sz)
{
	DBG("%s VSYNCS\n", msg);
	for(int i = 0; i < sz; i++) {
		DBG("%ld\n", va[i]);
	}
}

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

		if(server.recv_msg(&r, new_sockfd)) {
			ret = 1;
			break;
		}

	} while(r.get_type() != ACK);

	close(new_sockfd);
	return ret;
}

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

int do_secondary(char *server_name_or_ip_addr)
{
	msg m, r;
	int ret = 0;
	long client_vsync;
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

	if(get_vsync(&client_vsync, 1)) {
		return 1;
	}

	va = m.get_va();
	sz = m.get_size();

	print_vsyncs((char *) "PRIMARY'S", va, sz);
	print_vsyncs((char *) "SECONDARY'S", &client_vsync, 1);


	//	synchronize_vsync(1);
	return ret;
}

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
