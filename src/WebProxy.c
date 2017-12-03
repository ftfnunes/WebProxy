#include "HttpHandler.h"
#include "RequestValidator.h"
#include "WebCache.h"
#include "Log.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

/*void *handleSocket(void *arg) {
	Socket *socket = (Socket *)arg;
	HttpResponse *response = NULL;
	HttpRequest *request = httpReceive(socket);
	ValidationResult *validation = validateRequest(request);

	if (validation->isOnBlackList || (!validation->isOnWhiteList && validation->isOnDeniedTerm)){
		httpSendDeniedResult(socket, request, validation);
		close(socket)
		return NULL;
	}
	if (request->type != MethodType.PUT && request->type != MethodType.POST && request->type != MethodType.DELETE && request->type != MethodType.PATCH) {
		response = getResponseFromCache(request);
	}

	if (response == NULL) {
		response = httpSendRequest(request, socket);
		storeInCache(response, request);
	} else if(isExpired(response)) {
		removeFromCache(request)
		response = httpGetIfModified(request, response, socket);
		storeInCache(response, request);
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
}
*/
int main() {
	int srvSocket, rqstSocket;
	struct sockaddr_in* remote;
	struct sockaddr_in local;
	pthread_t threads[MAX_N_OF_CONNECTIONS];
	ThreadContext createdSockets[MAX_N_OF_CONNECTIONS];
	int nextThread = 0;
	unsigned int sockAddrSize = (sizeof(struct sockaddr_in));

	initializeLog();
	initializeCache();

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
