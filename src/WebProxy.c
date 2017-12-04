#include "HttpHandler.h"
#include "RequestValidator.h"
#include "WebCache.h"
#include "Log.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

extern int errno;

/*void *handleSocket(void *arg) {
	ThreadContext *context = (ThreadContext *)arg;
	HttpResponse *response = NULL;
	HttpRequest *request = httpReceive(context);
	ValidationResult *validation = validateRequest(request);

	if (validation->isOnBlackList || (!validation->isOnWhiteList && validation->isOnDeniedTerm)){
		httpSendDeniedResult(context, request, validation);
		close(context.socket);
		return NULL;
	}
	if (request->type != MethodType.PUT && request->type != MethodType.POST && request->type != MethodType.DELETE && request->type != MethodType.PATCH) {
		response = getResponseFromCache(request);
	}

	if (response == NULL) {
		response = httpSendRequest(request, context);
		storeInCache(response, request);
	} else if(isExpired(response)) {
		removeFromCache(request);
		response = httpGetIfModified(request, response, context);
		storeInCache(response, request);
	}

	if(!validation->isOnWhiteList) {
		validation = validateResponse(response);
		if (validation->isOnDeniedTerm){
			HttpSendDeniedResult(context, request, validation);
			close(context.socket);
			return NULL;
		}
	}

	HttpSendResponse(context, response);
	close(context.socket);
}
*/

void *handleSocket(void *arg) {
	int i;

	ThreadContext *context = (ThreadContext *)arg;
	HttpRequest *request;
	HttpResponse *response, *resp;

	logSuccess("Tentativa de conexao recebida.");
	request = httpReceiveRequest(context);
	//RequestPrettyPrinter(request);
	logSuccess("Requisicao de conexao recebida.");
	response = httpSendRequest(request);

	logSuccess("Resposta recebida.");
	//ResponsePrettyPrinter(response);
	//resp = httpParseResponse(response->raw);
	//ResponsePrettyPrinter(resp);

	HttpSendResponse(context, response);
	logSuccess("Dados de resposta enviados.");

	// free(response->version);
	// free(response->reasonPhrase);
	// free(response->body);
	// for (i = 0; i < response->headerCount; ++i){
	// 	free(response->headers[i].name);
	// 	free(response->headers[i].value);
	// }

	FreeHttpRequest(request);
	//FreeHttpResponse(resp);
	FreeHttpResponse(response);
	logSuccess("Conexao fechada.");
	printf("Conexao terminou.\n");
	freeResources(context);
	return NULL;
}

void sig_handler(int sigNum) {
	printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\nSIGBUS!!!\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
	exit(sigNum);
}

int main() {
	int srvSocket, rqstSocket;
	struct sockaddr_in* remote;
	struct sockaddr_in local;
	pthread_t threads[MAX_N_OF_CONNECTIONS];
	ThreadContext *context;
	int nextThread = 0;
	unsigned int sockAddrSize = (sizeof(struct sockaddr_in));
	struct timeval tv;


//	signal(SIGBUS, sig_handler);


	bzero(threads, MAX_N_OF_CONNECTIONS*sizeof(pthread_t));

	initializeLog();
	//initializeCache();

	configureSockAddr(&local, 32000, INADDR_ANY);
	srvSocket = socket(AF_INET, SOCK_STREAM, 0);
	int yes = 1;
	if ( setsockopt(srvSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1 ) {
    	printf("Options failed\n");
	}
	tv.tv_sec = 30;  /* 30 Secs Timeout */
	tv.tv_usec = 0;  // Not init'ing this can cause strange errors
	if (setsockopt(srvSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval)) != 0) {
		printf("Error setting timeout\n");
	}
	if (bind(srvSocket, (struct sockaddr*)&local, sizeof(struct sockaddr)) != 0) {
		printf("Binding error %d\n", errno);
		return 1;
	}
	listen(srvSocket, MAX_N_OF_CONNECTIONS);

	while(TRUE) {
		printf("\n\n\n\n\n\n\n\n\n\nThread %d waiting\n\n\n\n\n\n\n\n", nextThread);
		pthread_join(threads[nextThread], NULL);
		printf("\n\n\n\n\n\n\n\n\n\nThread %d starting\n\n\n\n\n\n\n\n", nextThread);
		remote = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
		rqstSocket = accept(srvSocket, (struct sockaddr*) remote, &sockAddrSize);
		printf("\n\n\n\n\n\n\n\nremote address = %ld\n\n\n\n\n\n\n", remote->sin_addr.s_addr);
		context = (ThreadContext *)malloc(sizeof(ThreadContext));

		context->sockAddr = remote;
		context->socket = rqstSocket;

		pthread_create(&threads[nextThread], NULL, handleSocket, context);
		++nextThread;
		if(nextThread > MAX_N_OF_CONNECTIONS-1){
			nextThread = 0;
		}
	}
	printf("\n\n\n\n\n\n\n\n\nWTF\n\n\n\n\n\n\n\n");
	return 0;
}
