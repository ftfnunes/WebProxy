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
	//int i;

	ThreadContext *context = (ThreadContext *)arg;
	HttpRequest *request;
	HttpResponse *response = NULL;
	ValidationResult *validation;

	//logSuccess("Tentativa de conexao recebida.");
	request = httpReceiveRequest(context);
	//RequestPrettyPrinter(request);
	//logSuccess("Requisicao de conexao recebida.");
	validation = ValidateRequest(request->hostname, request->body, request->bodySize, context->whitelist, context->blacklist, context->denyTerms);

	if(validation->isOnWhitelist){
		freeValidationResult(validation);

		if (request->type != PUT && request->type != POST && request->type != DELETE && request->type != PATCH) {
			response = getResponseFromCache(request);
		}

		if (response == NULL) {
			response = httpSendRequest(request);
			if (shouldBeCached(response)) {
				storeInCache(response, request);
			}
		} else if(isExpired(response)) {

			response = httpSendRequest(request);
			if (shouldBeCached(response)) {
				storeInCache(response, request);
			}
		}

		FreeHttpRequest(request);
		//logSuccess("Resposta recebida.");

		HttpSendResponse(context, response);
		FreeHttpResponse(response);
		//logSuccess("Dados de resposta enviados.");
		freeResources(context);
		pthread_exit(NULL);
	}

	if(validation->isOnBlacklist){
		freeValidationResult(validation);
		FreeHttpRequest(request);
		response = blacklistResponseBuilder();
		HttpSendResponse(context, response);
		FreeHttpResponse(response);
		freeResources(context);
		pthread_exit(NULL);
	}

	if(validation->isOnDeniedTerms){
		FreeHttpRequest(request);
		freeValidationResult(validation);
		response = deniedTermsResponseBuilder(validation, FALSE);
		HttpSendResponse(context, response);
		FreeHttpResponse(response);
		freeResources(context);
		pthread_exit(NULL);
	} else{
		freeValidationResult(validation);

		if (request->type != PUT && request->type != POST && request->type != DELETE && request->type != PATCH) {
			response = getResponseFromCache(request);
		}

		if (response == NULL) {
			response = httpSendRequest(request);
			if (shouldBeCached(response)) {
				storeInCache(response, request);
			}
		} else if(isExpired(response)) {
			response = httpSendRequest(request);
			if (shouldBeCached(response)) {
				storeInCache(response, request);
			}
		}

		FreeHttpRequest(request);
		//logSuccess("Resposta recebida.");

		validation = ValidateResponse(response->body, response->bodySize, context->denyTerms);

		if(validation->isOnDeniedTerms){
			FreeHttpResponse(response);
			freeValidationResult(validation);
			response = deniedTermsResponseBuilder(validation, TRUE);
			HttpSendResponse(context, response);
			FreeHttpResponse(response);
			freeResources(context);
			pthread_exit(NULL);
		} else{
			HttpSendResponse(context, response);
			FreeHttpResponse(response);
			//logSuccess("Dados de resposta enviados.");
			freeValidationResult(validation);
			freeResources(context);
			pthread_exit(NULL);
		}
	}

	return NULL;
}

void sig_handler(int sigNum) {
	printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\nSIGBUS!!!\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
	exit(sigNum);
}

int main(int argc, char **argv) {
	int srvSocket, rqstSocket;
	struct sockaddr_in* remote;
	struct sockaddr_in local;
	pthread_t threads[MAX_N_OF_CONNECTIONS];
	ThreadContext *context;
	int nextThread = 0;
	unsigned int sockAddrSize = (sizeof(struct sockaddr_in));
	struct timeval tv;
	List *whitelist, *blacklist, *denyTerms;
	int numOfConnections = MAX_N_OF_CONNECTIONS;
	int isInDebugMode = FALSE;

	if (argc > 1) {
		if (strcmp("debug", argv[1]) == 0) {
			isInDebugMode = TRUE;
		} else if (strcmp("-t", argv[1]) == 0) {
			if (argc != 3) {
				printf("Missing argumento for -t\n");
				exit(1);
			} else {
				numOfConnections = atoi(argv[2]);
				if (numOfConnections > MAX_N_OF_CONNECTIONS || numOfConnections < 1) {
					printf("The number of connections must be below %d\n", MAX_N_OF_CONNECTIONS);
					exit(1);
				}
			}
		} else {
			printf("Invalid argument\n");
			exit(1);
		}
	}

	if (isInDebugMode) {
		numOfConnections = 1;
	}


	bzero(threads, numOfConnections*sizeof(pthread_t));

	initializeLog();
	initializeCache();

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
	listen(srvSocket, numOfConnections);

	whitelist = getList("whitelist.txt");
	blacklist = getList("blacklist.txt");
	denyTerms = getList("denyTerms.txt");
	toLowerDenyTerms(denyTerms); /*transforma todos os deny terms em lower case, para comparar com o body, que tambem estara em lower case*/


	printf("Web Proxy started...\n");
	while(TRUE) {
		//printf("\n\n\n\n\n\n\n\n\n\nThread %d waiting\n\n\n\n\n\n\n\n", nextThread);
		pthread_join(threads[nextThread], NULL);
		//printf("\n\n\n\n\n\n\n\n\n\nThread %d starting\n\n\n\n\n\n\n\n", nextThread);
		remote = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
		rqstSocket = accept(srvSocket, (struct sockaddr*) remote, &sockAddrSize);
		//printf("\n\n\n\n\n\n\n\nremote address = %ld\n\n\n\n\n\n\n", remote->sin_addr.s_addr);
		context = (ThreadContext *)malloc(sizeof(ThreadContext));

		context->sockAddr = remote;
		context->socket = rqstSocket;
		context->whitelist = whitelist;
		context->blacklist = blacklist;
		context->denyTerms = denyTerms;

		pthread_create(&threads[nextThread], NULL, handleSocket, context);
		++nextThread;
		if(nextThread > numOfConnections-1){
			nextThread = 0;
		}
	}

	freeList(whitelist);
	freeList(blacklist);
	freeList(denyTerms);

	printf("\n\n\n\n\n\n\n\n\nWTF\n\n\n\n\n\n\n\n");
	return 0;
}
