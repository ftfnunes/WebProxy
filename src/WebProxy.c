#include "HttpHandler.h"
#include "RequestValidator.h"
#include "WebCache.h"
#include "Common.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

/*void *handleSocket(void *arg) {
	Socket *socket = (Socket *)arg;
	HttpResponse *response = NULL;
	HttpRequest *request = HttpReceive(socket);
	ValidationResult *validation = ValidateRequest(request);
	
	if (validation->isOnBlackList || (!validation->isOnWhiteList && validation->isOnDeniedTerm)){
		HttpSendDeniedResult(socket, request, validation);
		close(socket)
		return NULL;
	}
	if (request->type != MethodType.PUT && request->type != MethodType.POST && request->type != MethodType.DELETE && request->type != MethodType.PATCH) {
		response = getResponseFromCache(request);
	}
	
	if (response == NULL) {
		response = httpSendRequest(request, socket);
		storeInCache(Response, request);
	} else if(isExpired(response)) {
		response = httpGetIfModified(request, response, socket);
	}
	
	if(!validation->isOnWhiteList) {
		validation = validateResponse(response);
		if (validation->isOnDeniedTerm){
			HttpSendDeniedResult(socket, request, validation);
			close(socket);
			return NULL;
		}
	}
	
	HttpSendResponse(socket, response)
	close(socket)
}*/

int main() {
	int srvSocket, rqstSocket;
	struct sockaddr_in* remote;
	struct sockaddr_in local;
	pthread_t threads[MAX_N_OF_CONNECTIONS];
	Socket createdSockets[MAX_N_OF_CONNECTIONS];
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