// #include "HttpHandler.h"
// #include "RequestValidator.h"
// #include "WebCache.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_N_OF_CONNECTIONS 10 
#define TRUE 1

void configureSockAddr(struct sockaddr_in* sockAddr, int port, unsigned long addr) {
	bzero(sockAddr, sizeof(struct sockaddr_in));
	sockAddr->sin_family = AF_INET;
	sockAddr->sin_port = htons(port);
	sockAddr->sin_addr.s_addr = addr;
	bzero(&(sockAddr->sin_zero), 8);
}

typedef struct socket_str{
	struct sockaddr_in* sockAddr;
	int socket;
} t_socket;

int main() {
	int srvSocket, rqstSocket;
	struct sockaddr_in* remote;
	struct sockaddr_in local;
	pthread_t threads[MAX_N_OF_CONNECTIONS];
	t_socket createdSockets[MAX_N_OF_CONNECTIONS];
	int nextThread = 0;
	unsigned int sockAddrSize = (sizeof(struct sockaddr_in));

	configureSockAddr(&local, 1050, INADDR_ANY);
	srvSocket = socket(AF_INET, SOCK_STREAM, 0);
	bind(srvSocket, (struct sockaddr*)&local, sizeof(struct sockaddr));
	listen(srvSocket, MAX_N_OF_CONNECTIONS); 


	while(TRUE) {
		free(createdSockets[nextThread].sockAddr);

		remote = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
		rqstSocket = accept(srvSocket, (struct sockaddr*) remote, &sockAddrSize);
		
		createdSockets[nextThread].sockAddr = remote;
		createdSockets[nextThread].socket = rqstSocket;

		// pthread_create(&threads[nextThread], NULL, handleSocket, &createdSockets[nextThread]);
		nextThread++;
	}

	return 0;
}