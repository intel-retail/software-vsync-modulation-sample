#ifndef _DAEMON_SOCKET_H
#define _DAEMON_SOCKET_H

#include <netdb.h>
#include <string.h>
#include <linux/input.h>
#include <memory.h>
#include <vsyncalter.h>

#ifndef MAX_LEN
#define MAX_LEN 256
#endif

enum {
	SUCCESS,
	ERROR,
};

class connection {
private:
	int sockfd, portid;
	struct hostent * hostptr;
	sockaddr_in server_addr;
public:
	connection() : portid(5001) { };
	int init_client(char *server_name);
	int init_server();
	int open_socket();
	void set_server(int s_addr, int portid);
	void pr_inet(char **listptr, int length, char * buffer);
	int send_msg(void *m, int size, int sockid = 0);
	int recv_msg(void *m, int size, int sockid = 0);
	int accept_client(int *new_sockfd);
	void close_client();
	void close_server();
};


#endif
