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

#include "connection.h"
#include "message.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <fcntl.h> // For non-blocking sockets
#include <errno.h>
#include <ctype.h>

extern int client_done;

/**
 * @brief Construct a new connection::connection
 *
 * @param ip - Ip address for local machine (server mode)
 * 			   or for remote (client mode).
 */
connection::connection(const char* ip) : portid(5001) , con_type(TCP)
{
	sockfd = 0;
	hostptr = {};
	server_addr = {};
	
	if (ip) {
		strncpy(ip_address, ip, sizeof(ip_address) - 1);
		ip_address[sizeof(ip_address) - 1] = '\0'; // Ensure null-termination
	} else {
		ip_address[0] = '\0'; // Default to an empty string if no IP provided
	}
}

/**
* @brief
* This function opens a TCP socket stream
* @param type - TCP/UDP/PTP
* @return
* - 0 = SUCCESS
* - 1 = FAILURE
*/
int connection::open_socket(int type)
{
	sockfd = socket(type == PTP ? AF_PACKET : AF_INET, type == TCP ? SOCK_STREAM : SOCK_DGRAM, 0);

	if(sockfd < 0) {
		ERR("socket function failed! Error: %s\n", strerror(errno));
		return 1;
	}
	return 0;
}

/**
* @brief
* This function sets server characteristics like portid
* @param *addr - The address data structure that needs to be filled
* @param s_addr - server address
* @param portid - port id
* @return void
*/
void connection::set_server(sockaddr_in *addr, int s_addr, int portid)
{
	memset(addr, 0, sizeof(sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = s_addr;
	addr->sin_port = htons(portid);
}

/**
* @brief
* This function initalizes the client, by opening the socket and
* connecting to the server.
* @param *server_name - Either the server's hostname or its IP address
* @return
* - 0 = SUCCESS
* - 1 = FAILURE
*/
int connection::init_client(const char *server_name)
{
	TRACING();
	struct sockaddr_in serv_addr;
	struct hostent *server;

	if (!server_name) {
		ERR("Invalid server name provided\n");
		return 1;
	}

	if (open_socket(con_type)) {
		ERR("Failed to open socket\n");
		return 1;
	}

	server = gethostbyname(server_name);
	if (server == NULL) {
		ERR("No such host found\n");
		return 1;
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr,
		 (char *)&serv_addr.sin_addr.s_addr,
		 server->h_length);
	serv_addr.sin_port = htons(portid);

	if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		ERR("Error connecting: %s\n", strerror(errno));
		return 1;
	}
	return 0;
}

/**
* @brief
* Closes a client's connection to the server
* @param None
* @return void
*/
void connection::close_client()
{
	TRACING();
	close(sockfd);
}

/**
* @brief
* Closes a server's socket
* @param None
* @return void
*/
void connection::close_server()
{
	TRACING();
	close(sockfd);
}

/**
* @brief
* pr_inet
* @param **listptr
* @param length
* @param *buffer
* @return void
*/
void connection::pr_inet(char **listptr, int length, char * buffer)
{
	struct in_addr *ptr;
	while((ptr = (struct in_addr *) *listptr++) != NULL) {
		strncpy(buffer, inet_ntoa(*ptr), MAX_LEN-1);
	}
}

/**
* @brief
* This function initalizes the server by opening a socket,
* binding the server and listening for any incoming connections.
* @param None
* @return
* - 0 = SUCCESS
* - 1 = FAILURE
*/
int connection::init_server()
{
	TRACING();
	int ret, optval = 1;

	if(open_socket(con_type)) {
		return 1;
	}

	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) return 1;
	// Use stored IP address if provided, otherwise default to INADDR_ANY
	in_addr_t bind_addr = (ip_address[0] == '\0') ? htonl(INADDR_ANY) : inet_addr(ip_address);
	set_server(&server_addr, bind_addr, portid);

	ret = bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
	if(ret < 0) {
		ERR("bind function failed. errno is %d\n", errno);
		close_server();
		return 1;
	}

	ret = listen(sockfd, SOMAXCONN);
	if(ret < 0) {
		ERR("listen function failed.\n");
		close_server();
		return 1;
	}

	return 0;
}

/**
* @brief
* This function waits until a client connects to it. It uses
* the accept API to listen in for any incoming client connections.
* @param *new_sockfd - The socket id at which the client has connected. This will
* be used for future communication with the client.
* @return
* - 0 = SUCCESS
* - 1 = FAILURE
*/
int connection::accept_client(int *new_sockfd)
{
	TRACING();
	socklen_t fromlen;
	sockaddr_in from;

	fromlen = sizeof(from);
	*new_sockfd = accept(sockfd, (struct sockaddr *) &from, &fromlen);
	if(*new_sockfd < 0) {
		/*ERR("accept function failed\n"); */
		return 1;
	}
	INFO("Accepted client\n");

	return 0;
}

/**
* @brief
* This function sends a message to the other system using the send API
* @param *m - The message that we need to send
* @param size - The size of this message
* @param sockid - If not 0, this function will use this socket connection to
*	communicate with the other system on. If 0, then a global sockfd variable will
*	be used.
* @param *dest - The destination address
* @param dest_size - Size of the destination data structure
* @return
* - 0 = SUCCESS
* - 1 = FAILURE
*/
int connection::sendto_msg(void *m, int size, int sockid, struct sockaddr *dest, int dest_size)
{
	TRACING();
	long bytes_returned;
	bytes_returned = sendto(sockid ? sockid : sockfd,
		(char *) m,
		size,
		0,
		dest,
		dest_size);
	DBG("Sent message with %ld bytes\n", bytes_returned);
	if(bytes_returned != size) {
		ERR("send function failed. Error: %s\n", strerror(errno));
		return 1;
	}
	return 0;
}

/**
* @brief
* This function receives a message to the other system using the recv API
* @param *m - The message that we need to receive
* @param size - The size of this message
* @param sockid - If not 0, this function will use this socket connection to
*	communicate with the other system on. If 0, then a global sockfd variable will
*	be used.
* @param *dest - The destination address
* @param *dest_size - Size of the destination data structure
* @return
* - 0 = SUCCESS
* - 1 = FAILURE
*/
int connection::recvfrom_msg(void *m, int size, int sockid, struct sockaddr *dest, unsigned int *dest_size) {
	TRACING();
	ssize_t bytes_received;
	int sockfd_to_use = sockid ? sockid : sockfd;
	// Set the socket to non-blocking mode
	int flags = fcntl(sockfd_to_use, F_GETFL, 0);
	if (flags < 0) return 1;

	if (fcntl(sockfd_to_use, F_SETFL, flags | O_NONBLOCK) < 0) return 1;

	while (!client_done) {
		bytes_received = recvfrom(sockfd_to_use, (char *) m, size, 0, dest, (socklen_t *) dest_size);

		if (bytes_received > 0) {
			DBG("Received message with %ld bytes\n", bytes_received);
			return 0;
		} else if (bytes_received == 0) {
			// No data received, check if the client has closed the connection
			DBG("No data received, client may have closed the connection\n");
			return 1;
		} else {
			// An error occurred or the recvfrom call would block
			if (errno == EWOULDBLOCK || errno == EAGAIN) {
				// No data available right now, continue to poll
				const int us_in_ms=1000;
				usleep(100*us_in_ms); // Sleep for 100ms to prevent busy waiting
				continue;
			} else {
				ERR("recv function failed. Error: %s\n", strerror(errno));
				return 1;
			}
		}
	}
	return 0;
}

/**
* @brief
* Constructor of ptp_connection class. This just initializes a few class members
* @param *ifc - The PTP interface of the server. For examle: enp176s0
* @param *mac - The ethernet MAC address of the server. For example:
*		84:47:09:04:eb:0e
* @return void
*/
ptp_connection::ptp_connection(const char *ifc, const char *mac)
{
	iface_index = 0;
	con_type = PTP;
	memset(server_ip, 0, MAX_LEN);
	memset(iface, 0, MAX_LEN);

	strncpy(iface, ifc, MAX_LEN-1);
	if(mac) {
		strncpy(server_ip, mac, MAX_LEN-1);
	}
	memset(&dest_sa, 0, sizeof(dest_sa));
}

/**
* @brief
* Given a PTP interface name, this function finds the index
* of this interface. It does so by sending the SIOCGIFINDEX IOCTL to the
* network driver.
* @param *dest - destination string to be stored
* @param *src - source of the string to be copy over
* @param strBufferSize - size of the string buffer
* @return void
*/
void ptp_connection::safe_strncpy(char *dest, const char *src, size_t strBufferSize) {
    strncpy(dest, src, strBufferSize);
    // Ensure null termination
    if (strBufferSize > 0) {
        dest[strBufferSize - 1] = '\0';
    }
}

/**
* @brief
* Given a PTP interface name, this function finds the index
* of this interface. It does so by sending the SIOCGIFINDEX IOCTL to the
* network driver.
* @param *iface - The PTP interface of the server. For examle: enp176s0
* @return
* - 0 = SUCCESS
* - 1 = FAILURE
*/
int ptp_connection::find_iface_index(const char *iface)
{
	int sd;
	int err;
	struct ifreq ifreq;

	memset(&ifreq, 0, sizeof(ifreq));
	sd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if(sd == -1) {
		ERR("failed to open socket. Error: %s\n", strerror(errno));
		return 1;
	}

	safe_strncpy(ifreq.ifr_name, iface, IFNAMSIZ);
	err = ioctl(sd, SIOCGIFINDEX, &ifreq);
	if(err < 0) {
		ERR("failed to get interface index. Error: %s\n", strerror(errno));
		close(sd);
		return 1;
	}

	close(sd);
	return ifreq.ifr_ifindex;
}

#define MAX_MAC_ADDR_LEN 17
/**
* @brief
* This function converts a string based ethernet address to
* the l2 format. The result is stored in addr member variable
* @param *str - The address to convert.
* @return
* - true = SUCCESS
* - false = FAILURE
*/
bool ptp_connection::str_to_l2_addr(char *str)
{
	int i;
	char *save = str;
	size_t len = strnlen(str, MAX_MAC_ADDR_LEN);

	for(i = 0; i < ETH_ALEN; ++i) {
		unsigned long tmp;

		if(str >= save + len)
			break;
		tmp = strtoul(str, &str, 16);
		if(tmp == MAX_LEN)
			break;
		addr[i] = tmp;
		if(!ispunct(str[0]))
			break;
		++str;
	}

	return i == ETH_ALEN-1 ? true : false;
}

/**
* @brief
* This function opens a ptp socket, find the corresponding interface
* index and binds the connection.
* @param *iface
* @return
* - 0 = SUCCESS
* - 1 = FAILURE
*/
int ptp_connection::ptp_open(const char *iface)
{
	struct sockaddr_ll bind_arg;

	if(!iface) {
		ERR("Invalid interface\n");
		return 1;
	}

	if(open_socket(con_type)) {
		return 1;
	}

	iface_index = find_iface_index(iface);
	if(iface_index == -1) {
		ERR("Invalid interface index\n");
		return 1;
	}

	memset( &bind_arg, 0, sizeof(bind_arg));
	bind_arg.sll_family = AF_PACKET;
	bind_arg.sll_protocol = htons(portid);
	bind_arg.sll_ifindex = iface_index;
	if(bind(sockfd, (struct sockaddr *)&bind_arg, sizeof(bind_arg)) == -1) {
		ERR("Bind on socket failed. Error: %s\n", strerror(errno));
		return 1;
	}
	return 0;
}

/**
* @brief
* This function initializes the client. Besides opening a PTP
* socket, we also convert the ethernet address in the format that we need.
* @param *server_name
* @return
* - 0 = SUCCESS
* - 1 = FAILURE
*/
int ptp_connection::init_client(const char *server_name)
{
	TRACING();
	if(ptp_open(iface)) {
		return 1;
	}

	if(!str_to_l2_addr(server_ip)) {
		return 1;
	}

	dest_sa.sll_family  = AF_PACKET;
	dest_sa.sll_protocol = htons(portid);
	memcpy(dest_sa.sll_addr, addr, sizeof(addr));
	dest_sa.sll_ifindex = iface_index;
	dest_sa.sll_halen = ETH_ALEN;
	return 0;
}

/**
* @brief
* This function initializes the PTP server. It's just a wrapper
* around ptp_open.
* @param None
* @return
* - 0 = SUCCESS
* - 1 = FAILURE
*/
int ptp_connection::init_server()
{
	TRACING();
	if(ptp_open(iface)) {
		return 1;
	}
	return 0;
}

/**
* @brief
* This function sends a message to a destination address using
* sendto API.
* @param *m - The message to send
* @param size - The size of this message
* @param sockid - The file descriptor to send to
* @return
* - 0 = SUCCESS
* - 1 = FAILURE
*/
int ptp_connection::send_msg(void *m, int size, int sockid)
{
	TRACING();
	return sendto_msg(m, size, sockid, (struct sockaddr *) &dest_sa, sizeof(dest_sa));
}

/**
* @brief
* This function receives a message from a source address using
* recvfrom API.
* @param *m - The message to receive
* @param size - The size of this message
* @param sockid - The file descriptor to receive from
* @return
* - 0 = SUCCESS
* - 1 = FAILURE
*/
int ptp_connection::recv_msg(void *m, int size, int sockid)
{
	TRACING();

	socklen_t dest_sa_size = sizeof(dest_sa); // socklen_t is typically used for socket-related sizes
	return recvfrom_msg(m, size, sockid, (struct sockaddr *) &dest_sa, &dest_sa_size);
}
