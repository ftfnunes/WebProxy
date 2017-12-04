#include "HttpHandler.h"
#include "RequestValidator.h"
#include "WebCache.h"
#include "Log.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
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
	close(context->socket);

	free(context->sockAddr);
	free(context);
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

	signal(SIGBUS, sig_handler);


	bzero(threads, MAX_N_OF_CONNECTIONS*sizeof(pthread_t));

	initializeLog();
	//initializeCache();

	configureSockAddr(&local, 32000, INADDR_ANY);
	srvSocket = socket(AF_INET, SOCK_STREAM, 0);
	int yes = 1;
	if ( setsockopt(srvSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1 ) {
    	printf("Options failed\n");
	}
	if (bind(srvSocket, (struct sockaddr*)&local, sizeof(struct sockaddr)) != 0) {
		printf("Binding error %d\n", errno);
		return 1;
	}
	listen(srvSocket, MAX_N_OF_CONNECTIONS);

	while(TRUE) {
		pthread_join(threads[nextThread], NULL);
		remote = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
		rqstSocket = accept(srvSocket, (struct sockaddr*) remote, &sockAddrSize);

		context = (ThreadContext *)malloc(sizeof(ThreadContext));

		context->sockAddr = remote;
		context->socket = rqstSocket;

		pthread_create(&threads[nextThread], NULL, handleSocket, context);
		++nextThread;
		if(nextThread > MAX_N_OF_CONNECTIONS-1){
			nextThread = 0;
		}
	}

	return 0;
}
