#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>

#ifndef COMMON_INC

#define COMMON_INC
#define MAX_N_OF_CONNECTIONS 10 
#define TRUE 1
#define FALSE 0

typedef struct socket_str{
	struct sockaddr_in* sockAddr;
	int socket;
} ThreadContext;

void configureSockAddr(struct sockaddr_in* sockAddr, int port, unsigned long addr);
#endif