#include "connection.h"
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
#include <errno.h>

int connection::open_socket()
{
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if(sockfd < 0) {
		ERR("socket function failed!!\n");
		return 1;
	}
	return 0;
}

void connection::set_server(int s_addr, int portid)
{
	memset((char *)&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = s_addr;
	server_addr.sin_port = htons(portid);
}

int connection::init_client(char *server_name)
{
	char server_host_addr[MAX_LEN], copy[MAX_LEN], *ptr;
	int ret;
	in_addr_t data;

	if(!server_name) {
		ERR("Invalid server name\n");
		return 1;
	}

	if(open_socket()) {
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
	set_server(inet_addr(server_host_addr), portid);

	ret = connect(sockfd,
		(struct sockaddr *) &server_addr,
		sizeof(server_addr));

	if(ret < 0) {
		ERR("Couldn't connect to the server\n");
		return 1;
	}
	return 0;
}

void connection::close_client()
{
	msg m;
	m.close();
	send_msg(&m);
	recv_msg(&m);
	close(sockfd);
}

void connection::close_server()
{
	close(sockfd);
}

void connection::pr_inet(char **listptr, int length, char * buffer)
{
	struct in_addr *ptr;
	while((ptr = (struct in_addr *) *listptr++) != NULL) {
		strcpy(buffer, inet_ntoa(*ptr));
	}
}

int connection::init_server()
{
	int ret, optval = 1;

	if(open_socket()) {
		return 1;
	}

	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	set_server(htonl(INADDR_ANY), portid);

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

int connection::send_msg(msg *m, int sockid)
{
	long bytes_returned;
	bytes_returned = send(sockid ? sockid : sockfd,
		(char *) m,
		sizeof(*m),
		0);
	if(bytes_returned != sizeof(*m)) {
		ERR("send function failed\n");
		return 1;
	}
	return 0;
}

int connection::recv_msg(msg *m, int sockid)
{
	long bytes_returned;
	bytes_returned = recv(sockid ? sockid : sockfd,
		(char *) m,
		sizeof(*m),
		0);
	if(bytes_returned < 0) {
		ERR("recv function failed\n");
		return 1;
	}
	return 0;
}
