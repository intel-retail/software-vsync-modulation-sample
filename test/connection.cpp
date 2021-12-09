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
#include <linux/if_packet.h>
#include <errno.h>
#include <ctype.h>

/*******************************************************************************
 * Description
 *	open_socket - This function opens a TCP socket stream
 * Parameters
 *	int type - TCP/UDP/PTP
 * Return val
 *	int - 0 = SUCCESS, 1 = FAILURE
 ******************************************************************************/
int connection::open_socket(int type)
{
	sockfd = socket(type == PTP ? AF_PACKET : AF_INET, type == TCP ? SOCK_STREAM : SOCK_DGRAM, 0);

	if(sockfd < 0) {
		ERR("socket function failed! Error: %s\n", strerror(errno));
		return 1;
	}
	return 0;
}

/*******************************************************************************
 * Description
 *	set_server - This function sets server characteristics like portid
 * Parameters
 *  sockaddr_in *addr - The address data structure that needs to be filled
 *	int s_addr - server address
 *	int portid - port id
 * Return val
 *	void
 ******************************************************************************/
void connection::set_server(sockaddr_in *addr, int s_addr, int portid)
{
	memset(addr, 0, sizeof(sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = s_addr;
	addr->sin_port = htons(portid);
}

/*******************************************************************************
 * Description
 *	init_client - This function initalizes the client, by opening the socket and
 *	connecting to the server.
 * Parameters
 *	const char *server_name - Either the server's hostname or its IP address
 * Return val
 *	int - 0 = SUCCESS, 1 = FAILURE
 ******************************************************************************/
int connection::init_client(const char *server_name)
{
	char server_host_addr[MAX_LEN], copy[MAX_LEN], *ptr;
	int ret;
	in_addr_t data;

	if(!server_name) {
		ERR("Invalid server name\n");
		return 1;
	}

	if(open_socket(con_type)) {
		return 1;
	}

	strcpy(copy, server_name);
	ptr = strtok(copy, ".");

	if(atoi(ptr)) {
		data = inet_addr(server_name);
		hostptr = gethostbyaddr(&data, 4, AF_INET);
	} else {
		hostptr = gethostbyname(server_name);
	}

	if(!hostptr) {
		ERR("Invalid Host name\n");
		return 1;
	}

	pr_inet(hostptr->h_addr_list, hostptr->h_length, server_host_addr);
	set_server(&server_addr, inet_addr(server_host_addr), portid);

	ret = connect(sockfd,
		(struct sockaddr *) &server_addr,
		sizeof(server_addr));

	if(ret < 0) {
		ERR("Couldn't connect to the server\n");
		return 1;
	}
	return 0;
}

/*******************************************************************************
 * Description
 *	close_client - Closes a client's connection to the server
 * Parameters
 *	NONE
 * Return val
 *	void
 ******************************************************************************/
void connection::close_client()
{
	msg m;
	m.close();
	send_msg(&m, sizeof(m));
	recv_msg(&m, sizeof(m));
	close(sockfd);
}

/*******************************************************************************
 * Description
 *	close_server - Closes a server's socket
 * Parameters
 *	NONE
 * Return val
 *	void
 ******************************************************************************/
void connection::close_server()
{
	close(sockfd);
}

/*******************************************************************************
 * Description
 *	pr_inet
 * Parameters
 *	char **listptr
 *	int length
 *	char * buffer
 * Return val
 *	void
 ******************************************************************************/
void connection::pr_inet(char **listptr, int length, char * buffer)
{
	struct in_addr *ptr;
	while((ptr = (struct in_addr *) *listptr++) != NULL) {
		strcpy(buffer, inet_ntoa(*ptr));
	}
}

/*******************************************************************************
 * Description
 *	init_server - This function initalizes the server by opening a socket,
 *	binding the server and listening for any incoming connections.
 * Parameters
 *	NONE
 * Return val
 *	int - 0 = SUCCESS, 1 = FAILURE
 ******************************************************************************/
int connection::init_server()
{
	int ret, optval = 1;

	if(open_socket(con_type)) {
		return 1;
	}

	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	set_server(&server_addr, htonl(INADDR_ANY), portid);

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

/*******************************************************************************
 * Description
 *	accept_client - This function waits until a client connects to it. It uses
 *	the accept API to listen in for any incoming client connections.
 * Parameters
 *	int *new_sockfd - The socket id at which the client has connected. This will
 *	be used for future communication with the client.
 * Return val
 *	int - 0 = SUCCESS, 1 = FAILURE
 ******************************************************************************/
int connection::accept_client(int *new_sockfd)
{
	socklen_t fromlen;
	sockaddr_in from;

	fromlen = sizeof(from);
	*new_sockfd = accept(sockfd, (struct sockaddr *) &from, &fromlen);
	if(*new_sockfd < 0) {
		/*ERR("accept function failed\n"); */
		return 1;
	}

	return 0;
}

/*******************************************************************************
 * Description
 *	sendto_msg - This function sends a message to the other system using the send API
 * Parameters
 *	void *m - The message that we need to send
 *	int size - The size of this message
 *	int sockid - If not 0, this function will use this socket connection to
 *	communicate with the other system on. If 0, then a global sockfd variable will
 *	be used.
 *	struct sockaddr *dest - The destination address
 *	int dest_size - Size of the destination data structure
 * Return val
 *	int - 0 = SUCCESS, 1 = FAILURE
 ******************************************************************************/
int connection::sendto_msg(void *m, int size, int sockid, struct sockaddr *dest, int dest_size)
{
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

/*******************************************************************************
 * Description
 *	recvfrom_msg - This function receives a message to the other system using the
 *	recv API
 * Parameters
 *	void *m - The message that we need to receive
 *	int size - The size of this message
 *	int sockid - If not 0, this function will use this socket connection to
 *	communicate with the other system on. If 0, then a global sockfd variable will
 *	be used.
 *	struct sockaddr *dest - The destination address
 *	int dest_size - Size of the destination data structure
 * Return val
 *	int - 0 = SUCCESS, 1 = FAILURE
 ******************************************************************************/
int connection::recvfrom_msg(void *m, int size, int sockid, struct sockaddr *dest, int dest_size)
{
	long bytes_returned;
	int cli_len = dest_size;
	bytes_returned = recvfrom(sockid ? sockid : sockfd,
			(char *) m,
			size,
			0,
			dest,
			(unsigned int *) &cli_len);
	DBG("Received message with %ld bytes\n", bytes_returned);
	if(bytes_returned < 0) {
		ERR("recv function failed. Error: %s\n", strerror(errno));
		return 1;
	}
	return 0;
}
