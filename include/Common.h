#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>

#ifndef COMMON

#define COMMON
#define MAX_N_OF_CONNECTIONS 10 
#define TRUE 1
#define FALSE 0

typedef struct socket_str{
	struct sockaddr_in* sockAddr;
	int socket;
} Socket;

void configureSockAddr(struct sockaddr_in* sockAddr, int port, unsigned long addr) {
	bzero(sockAddr, sizeof(struct sockaddr_in));
	sockAddr->sin_family = AF_INET;
	sockAddr->sin_port = htons(port);
	sockAddr->sin_addr.s_addr = addr;
	bzero(&(sockAddr->sin_zero), 8);
}

#endif