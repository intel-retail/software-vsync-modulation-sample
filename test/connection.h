#ifndef _DAEMON_SOCKET_H
#define _DAEMON_SOCKET_H

#include <netdb.h>
#include <string.h>
#include <linux/input.h>
#include <memory.h>
#include <vsyncalter.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>

#ifndef MAX_LEN
#define MAX_LEN 256
#endif

enum {
	SUCCESS,
	ERROR,
};

enum {
	TCP,
	UDP,
	PTP,
};

class connection {
protected:
	int sockfd, portid, con_type;
	struct hostent * hostptr;
	sockaddr_in server_addr;
	void set_server(sockaddr_in *addr, int s_addr, int portid);
	void pr_inet(char **listptr, int length, char * buffer);
public:
	connection() : portid(5001) , con_type(TCP) { };
	virtual ~connection() {}
	virtual int init_client(const char *server_name);
	virtual int init_server();
	int open_socket(int type);
	int sendto_msg(void *m, int size, int sockid, struct sockaddr *dest, int dest_size);
	int recvfrom_msg(void *m, int size, int sockid, struct sockaddr *dest, int dest_size);
	virtual int send_msg(void *m, int size, int sockid = 0) { return sendto_msg(m, size, sockid, (struct sockaddr *) &server_addr, sizeof(server_addr)); }
	virtual int recv_msg(void *m, int size, int sockid = 0) { return recvfrom_msg(m, size, sockid, (struct sockaddr *) &server_addr, sizeof(server_addr)); }
	virtual int accept_client(int *new_sockfd);
	void close_client();
	void close_server();
};


#endif
