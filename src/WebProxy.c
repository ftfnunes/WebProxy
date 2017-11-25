#include "HttpHandler.h"
#include "RequestValidator.h"
#include "WebCache.h"
#include "Common.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

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