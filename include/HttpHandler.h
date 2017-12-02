#include <stdio.h>
#include <stdlib.h>
#include "Common.h"

#ifndef HTTP_HANDLER

#define HTTP_HANDLER
typedef enum methodType{
	GET,
	POST,
	DELETE,
	PUT,
	HEAD,
	OPTIONS,
	TRACE,
	CONNECT,
	PATCH
} MethodType;

typedef struct headerField{
	char *name;
	char *value;
} HeaderField;

typedef struct httpRequest{
	char *method;
	MethodType type;
	char *uri;
	char *version;
	char *hostname;
	char *body;
	char *raw;
	/* Vetor de headers alocados dinâmicamente, usando o headerCount para controle.*/
	HeaderField *headers;
	int headerCount;
} HttpRequest;

typedef struct httpResponse{
	char *version;
	short statusCode;
	char *reasonPhrase;
	char *body;
	char *raw;
	/* Vetor de headers alocados dinâmicamente, usando o headerCount para controle.*/
	HeaderField *headers;
	int headerCount;
} HttpResponse;


HttpResponse *httpReceiveResponse(ThreadContext *context);
HttpRequest *httpReceiveRequest(ThreadContext *context);

void ResponsePrettyPrinter(HttpResponse *response);
void RequestPrettyPrinter(HttpRequest *request);

HeaderField *getHeaders(ThreadContext *context, char **raw, int *headerCount, int *req_size, int *has_body, int *body_size, char **hostname);
HeaderField *getLocalHeaders(char *resp, int *length, char **raw, int *headerCount, int *req_size, int *has_body, int *body_size, char **hostname);
char *getBody(ThreadContext *context, char **raw, int *req_size, int body_size);
char *getLocalBody(char *resp, int *length, char **raw, int *req_size, int body_size);

HttpResponse *httpParseResponse(char *response);



#endif