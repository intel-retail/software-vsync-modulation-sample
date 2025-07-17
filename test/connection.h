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
	char ip_address[32];  // Store IP address
	void set_server(sockaddr_in *addr, int s_addr, int portid);
	void pr_inet(char **listptr, int length, char * buffer);
public:
	connection(const char* ip = "");
	virtual ~connection() {}
	virtual int init_client(const char *server_name);
	virtual int init_server();
	int open_socket(int type);
	int sendto_msg(void *m, int size, int sockid, struct sockaddr *dest, int dest_size);
	int recvfrom_msg(void *m, int size, int sockid, struct sockaddr *dest, unsigned int *dest_size);
	virtual int send_msg(void *m, int size, int sockid = 0) { return sendto_msg(m, size, sockid, (struct sockaddr *) &server_addr, sizeof(server_addr)); }
	virtual int recv_msg(void *m, int size, int sockid = 0)
	{
		struct sockaddr_in dest_sa; // Assuming dest_sa is of type sockaddr_in
		socklen_t dest_sa_size = sizeof(dest_sa); // socklen_t is typically used for socket-related sizes
		return recvfrom_msg(m, size, sockid, (struct sockaddr *) &server_addr, &dest_sa_size);
	}
	virtual int accept_client(int *new_sockfd);
	void close_client();
	void close_server();
};


class ptp_connection : public connection {
protected:
	char iface[MAX_LEN];
	char server_ip[MAX_LEN];
	uint8_t addr[ETH_ALEN];
	int iface_index;
	sockaddr_ll dest_sa;
	int ptp_open(const char *iface);
	int find_iface_index(const char *iface);
	bool str_to_l2_addr(char *str);
	void safe_strncpy(char *dest, const char *src, size_t strBufferSize);
public:
	ptp_connection(const char *ifc, const char *mac = NULL);
	int init_client(const char *server_name);
	int init_server();
	int send_msg(void *m, int size, int sockid);
	int recv_msg(void *m, int size, int sockid = 0);
	int accept_client(int *new_sockfd) { *new_sockfd = 0; return 0; }
};


#endif
