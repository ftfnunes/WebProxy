#include "HttpHandler.h"
#include "RequestValidator.h"
#include "WebCache.h"
#include "Common.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HttpResponse *httpReceiveResponse(ThreadContext *context){
	int length, size, body_size = 0, req_size, has_body = 0;

	/* Buffer de 3000, pois foi pesquisado que URL's com mais de 2000 caracteres podem não funcionar em todos as conexões cliente-servidor.*/
	char buffer[3000], buff;
	HttpResponse *response = (HttpResponse *)malloc(sizeof(HttpResponse));

	response->version = NULL;
	response->statusCode = -1;
	response->reasonPhrase = NULL;
	response->body = NULL;
	response->raw = NULL;
	response->headers = NULL;
	response->headerCount = 0;

	req_size = 0;
	size = 0;
	bzero(buffer, 3000);


	while(1){
		length = recv(context->socket, &buff, 1, 0);
		if(length < 1){
			printf("Error on receiving data from socket. Size < 0.\n");
			exit(1);
		}

		response->raw = (char *)realloc(response->raw, (req_size+1)*sizeof(char));
		response->raw[req_size] = buff;
		++req_size;

		if(buff == '\n'){
			bzero(buffer, 3000);
			break;
		}

		if(buff == ' ' || buff == '\r'){
			if(response->version == NULL){
				response->version = (char *)malloc((size+1)*sizeof(char));
				strcpy(response->version, buffer);
				response->version[size] = '\0';
				//printf("Version: %s\n", request->version);
			} else if(response->statusCode == -1){
				response->statusCode = (short)atoi(buffer);
				//printf("Status Code: %d\n", request->statusCode);
			} else if(buff == '\r'){
				response->reasonPhrase = (char *)malloc((size+1)*sizeof(char));
				strcpy(response->reasonPhrase, buffer);
				response->reasonPhrase[size] = '\0';
				//printf("Reason Phrase: %s\n", request->reasonPhrase);
			}

			bzero(buffer, 3000);
			size = 0;
		} else{
			buffer[size] = buff;
			++size;
		}
	}

	response->headers = getHeaders(context, &(response->raw), &(response->headerCount), &req_size, &has_body, &body_size, NULL);
	

	if( has_body == 1 ){
		response->body = getBody(context, &(response->raw), &req_size, body_size);
	}

	ResponsePrettyPrinter(response);

	return response;
}


HttpRequest *httpReceiveRequest(ThreadContext *context){
	int length, size, body_size = 0, req_size, has_body = 0;

	/* Buffer de 3000, pois foi pesquisado que URL's com mais de 2000 caracteres podem não funcionar em todos as conexões cliente-servidor.*/
	char buffer[3000], buff;
	HttpRequest *request = (HttpRequest *)malloc(sizeof(HttpRequest));

	request->method = NULL;
	request->uri = NULL;
	request->version = NULL;
	request->hostname = NULL;
	request->body = NULL;
	request->raw = NULL;
	request->headerCount = 0;
	request->headers = NULL;

	req_size = 0;
	size = 0;
	bzero(buffer, 3000);

	// while(recv(context->socket, &buff, 1, 0) > 0){
	// 	printf("buff: '%c'\n", buff);
	// }
	// return;

	while(1){
		length = recv(context->socket, &buff, 1, 0);
		if(length < 1){
			printf("Error on receiving data from socket. Size < 0.\n");
			exit(1);
		}

		request->raw = (char *)realloc(request->raw, (req_size+1)*sizeof(char));
		request->raw[req_size] = buff;
		++req_size;

		if(buff == '\n'){
			bzero(buffer, 3000);
			break;
		}

		if(buff == ' ' || buff == '\r'){
			if(request->method == NULL){
				request->method = (char *)malloc((size+1)*sizeof(char));
				strcpy(request->method, buffer);
				request->method[size] = '\0';
				//printf("Method: %s\n", request->method);
			} else{
				if(request->uri == NULL){
					request->uri = (char *)malloc((size+1)*sizeof(char));
					strcpy(request->uri, buffer);
					request->uri[size] = '\0';
					//printf("Uri: %s\n", request->uri);
				} else{
					request->version = (char *)malloc((size+1)*sizeof(char));
					strcpy(request->version, buffer);
					request->version[size] = '\0';
					//printf("Version: %s\n", request->version);
				}
			}

			bzero(buffer, 3000);
			size = 0;
		} else{
			buffer[size] = buff;
			++size;
		}
	}

	if(strcmp(request->method, "GET") == 0){
		request->type = GET;
	} else if(strcmp(request->method, "POST") == 0) {
		request->type = POST;
	} else if(strcmp(request->method, "DELETE") == 0) {
		request->type = DELETE;
	} else if(strcmp(request->method, "PUT") == 0) {
		request->type = PUT;
	} else if(strcmp(request->method, "HEAD") == 0) {
		request->type = HEAD;
	} else if(strcmp(request->method, "OPTIONS") == 0) {
		request->type = OPTIONS;
	} else if(strcmp(request->method, "TRACE") == 0) {
		request->type = TRACE;
	} else if(strcmp(request->method, "CONNECT") == 0) {
		request->type = CONNECT;
	} else if(strcmp(request->method, "PATCH") == 0) {
		request->type = PATCH;
	}

	printf("Type: %d\nBuff: '%c'\n", request->type, buff);
	// has_body = 1;


	request->headers = getHeaders(context, &(request->raw), &(request->headerCount), &req_size, &has_body, &body_size, &request->hostname);
	

	if( has_body == 1 ){
		request->body = getBody(context, &(request->raw), &req_size, body_size);
	}

	RequestPrettyPrinter(request);

	return request;
}

HeaderField *getHeaders(ThreadContext *context, char **raw, int *headerCount, int *req_size, int *has_body, int *body_size, char **hostname){
	int length, found_linebreak = 0, size = 0, found_name;
	char buff, buffer[3000], *value, *name;
	HeaderField *headers;

	printf("Entrou.\n");

	while(1){
		length = recv(context->socket, &buff, 1, 0);
		if(length < 1){
			printf("Error on receiving data from socket. Size < 0.\n");
			exit(1);
		}

		(*raw) = (char *)realloc((*raw), ((*req_size)+1)*sizeof(char));
		(*raw)[(*req_size)] = buff;
		(*req_size) = (*req_size) + 1;
		printf("buff: '%c'\n", buff);	


		if(found_linebreak == 1 && buff == '\r'){
			++found_linebreak;
			continue;
		} else if(found_linebreak == 2 && buff == '\n'){
			break;
		} else {
			found_linebreak = 0;
		}


		if(buff == '\n'){
			headers = (HeaderField *)realloc(headers, ((*headerCount)+1)*sizeof(HeaderField));
			headers[*headerCount].name = name;
			headers[*headerCount].value = value;
			++(*headerCount);
			
			found_linebreak = 1;
			size = 0;
			bzero(buffer, 3000);
			continue;
		}

		if(buff == ' ' && found_name == 0){
			name = (char *)malloc(size*sizeof(char));
			strcpy(name, buffer);
			name[size-1] = '\0';
			found_name = 1;

			//printf("name: '%s'\n", name);
			
			bzero(buffer, 3000);
			size = 0;
		} else if(buff == '\r' && found_name == 1){
			value = (char *)malloc((size+1)*sizeof(char));
			strcpy(value, buffer);
			value[size] = '\0';

			//printf("value: '%s'\n", value);
			if(strcmp(name, "Content-Length") == 0){
				*body_size = atoi(value);
				*has_body = 1;
			}
			if(strcmp(value, "Host") == 0 && hostname != NULL){
				*hostname = value;
			}
			
			found_name = 0;
			bzero(buffer, 3000);
			size = 0;
		} else {
			//printf("Char: '%c'\n", buff);
			buffer[size] = buff;
			++size;
		}
	}

	return headers;
}

char *getBody(ThreadContext *context, char **raw, int *req_size, int body_size){
	int length, size;
	char *body;

	body = (char *)malloc((body_size+1)*sizeof(char));

	length = recv(context->socket, body, body_size, 0);
	if(length < 0){
		printf("Error on receiving data from socket. Size < 0.\n");
		exit(1);
	}
	
	body[body_size] = '\0';
	
	(*raw) = (char *)realloc((*raw), ((*req_size)+body_size+1)*sizeof(char));
	
	for (size = 0; size < body_size; ++size){
		(*raw)[(*req_size)] = body[size];
		++(*req_size);
	}
	(*raw)[(*req_size)] = '\0';
	++(*req_size);

	return body;
}





HttpResponse httpSendRequest(HttpRequest request, ThreadContext *context){
	HttpResponse response;

	return response;
}



HttpResponse *httpParseResponse(char *response) {
	return NULL;
}

void ResponsePrettyPrinter(HttpResponse *response){
	int i;

	printf("Version: %s\n", response->version);
	printf("Status Code: %d\n", response->statusCode);
	printf("Reason Phrase: %s\n\n", response->reasonPhrase);


	for(i = 0; i < response->headerCount; ++i){
		printf("Name: %s\nValue: %s\n\n", response->headers[i].name, response->headers[i].value);
	}

	if(response->body != NULL){
		printf("Body: %s\n", response->body);
	}

	printf("Raw: %s\n", response->raw);
}

void RequestPrettyPrinter(HttpRequest *request){
	int i;

	printf("Method: %s\n", request->method);
	printf("Uri: %s\n", request->uri);
	printf("Version: %s\n\n", request->version);
	printf("Type: %d\n", request->type);
	printf("Header Count: %d\n", request->headerCount);

	for(i = 0; i < request->headerCount; ++i){
		printf("Name: %s\nValue: %s\n\n", request->headers[i].name, request->headers[i].value);
	}

	if(request->body != NULL){
		printf("Body: %s\n", request->body);
	}

	printf("Raw: %s\n", request->raw);

}