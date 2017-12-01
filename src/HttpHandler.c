#include "HttpHandler.h"
#include "RequestValidator.h"
#include "WebCache.h"
#include "Common.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HttpRequest httpReceive(Socket *context){
	int length, size, found_linebreak = 0, body_size = 0, req_size = 0;

	/* Buffer de 3000, pois foi pesquisado que URL's com mais de 2000 caracteres podem não funcionar em todos as conexões cliente-servidor.*/
	char buffer[3000], buff, *name, *value;
	HttpRequest request;

	request.method = NULL;
	request.uri = NULL;
	request.version = NULL;
	request.hostname = NULL;
	request.body = NULL;
	request.raw = NULL;
	request.headerCount = 0;

	request.raw = (char *)malloc(sizeof(char));


	size = 0;
	bzero(buffer, 3000);

	while(1){
		length = recv(context->socket, &buff, 1, 0);
		if(length < 1){
			printf("Error on receiving data from socket. Size < 0.\n");
			exit(1);
		}

		request.raw[req_size] = buff;
		++req_size;
		request.raw = (char *)realloc(request.raw, req_size+1);

		if(buff == '\n'){
			bzero(buffer, 3000);
			break;
		}

		if(buff == ' ' || buff == '\r'){
			if(request.method == NULL){
				request.method = (char *)malloc((size+1)*sizeof(char));
				strcpy(request.method, buffer);
				request.method[size] = '\0';
			} else{
				if(request.uri == NULL){
					request.uri = (char *)malloc((size+1)*sizeof(char));
					strcpy(request.uri, buffer);
					request.uri[size] = '\0';
				} else{
					request.version = (char *)malloc((size+1)*sizeof(char));
					strcpy(request.version, buffer);
					request.version[size] = '\0';
				}
			}

			bzero(buffer, 3000);
			size = 0;
		} else{
			buffer[size] = buff;
			++size;
		}
	}

	if(strcmp(request.method, "GET") == 0){
		request.type = GET;
	} else if(strcmp(request.method, "POST") == 0) {
		request.type = POST;
	} else if(strcmp(request.method, "DELETE") == 0) {
		request.type = DELETE;
	} else if(strcmp(request.method, "PUT") == 0) {
		request.type = PUT;
	} else if(strcmp(request.method, "HEAD") == 0) {
		request.type = HEAD;
	} else if(strcmp(request.method, "OPTIONS") == 0) {
		request.type = OPTIONS;
	} else if(strcmp(request.method, "TRACE") == 0) {
		request.type = TRACE;
	} else if(strcmp(request.method, "CONNECT") == 0) {
		request.type = CONNECT;
	} else if(strcmp(request.method, "PATCH") == 0) {
		request.type = PATCH;
	}




	

	size = 0;
	bzero(buffer, 3000);
	while(1){
		length = recv(context->socket, &buff, 1, 0);
		if(length < 1){
			printf("Error on receiving data from socket. Size < 0.\n");
			exit(1);
		}

		request.raw[req_size] = buff;
		++req_size;
		request.raw = (char *)realloc(request.raw, req_size+1);


		if(buff == '\n'){
			if(request.headerCount == 0){
				request.headers = (HeaderField *)malloc(sizeof(HeaderField));
			} else {
				request.headers = (HeaderField *)realloc(request.headers, (request.headerCount+1)*sizeof(HeaderField));
			}
			request.headers[request.headerCount].name = name;
			request.headers[request.headerCount].value = value;
			++(request.headerCount);
			

			bzero(buffer, 3000);
		}

		if(buff == ' '){
			header = (HeaderField *)malloc(sizeof(HeaderField));
			name = (char *)malloc(size*sizeof(char));
			name[size-1] = '\0';
			
			bzero(buffer, 3000);
			size = 0;
		} else if(buff == '\r'){
			value = (char *)malloc((size+1)*sizeof(char));
			value[size] = '\0';
			if(strcmp(value, "Content-Length") == 0){
				body_size = atoi(value);
			}
			if(strcmp(value, "Host") == 0){
				request.hostname = value;
			}
			
			bzero(buffer, 3000);
			size = 0;
		} else {
			buffer[size] = buff;
			++size;
		}
	}








	// length = recv(context->socket, buffer, 3000, 0);
	// if(length < 0){
	// 	printf("Error on receiving data from socket. Size < 0.\n");
	// 	exit(1);
	// }
	
	// for(i = 0; i < 3000; ++i){
	// 	if(buffer[i] == '\n'){
	// 		found_linebreak = 1;
	// 		break;
	// 	}

	// 	if(buffer[i] != ' ' && start_field != -1){
	// 		start_field = i;
	// 	}

	// 	if(method == null && buffer[i] ==  ' '){
	// 		field_size = i-start_field;
	// 		request.method = (char *)malloc((field_size+1)*sizeof(char));
	// 		memcpy(request.method, &buffer[start_field], i-start_field);
	// 		request.method[field_size] = '\0';
	// 		start_field = -1;
	// 	}

	// 	if(i == 2999 && found_linebreak == 0){

	// 	}
	// }


	/* Só será possível sair do laço quando o padrão de saída da requisição for encontrado e o comando 'break' for executado. */
	while(1){
		length = recv(context->socket, buffer, 1000, 0);
		if(length < 0){
			printf("Error on receiving data from socket. Size < 0.\n");
			exit(1);
		}


	}







	return request;
}

HttpResponse httpSendRequest(HttpRequest request, Socket *context){
	HttpResponse response;

	return response;
}



HttpResponse *httpParseResponse(char *response) {
	return NULL;
}