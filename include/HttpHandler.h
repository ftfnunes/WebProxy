#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "Common.h"
#include "Log.h"

#ifndef HTTP_HANDLER

#define HTTP_HANDLER

/*
	Enumeração para os tipos de requisição.
*/
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

/*
	Struct para o armazenamento de um Header. "Name" representa o campo do header e "Value" o conteúdo de um header. 
*/
typedef struct headerField{
	char *name;
	char *value;
} HeaderField;

/*
	Estrutura que armazena uma Requisição Http.
*/
typedef struct httpRequest{
	char *method; /* Método da requisição. */
	MethodType type; /* Enum que representa o tipo da requisição. */
	char *uri; /* Uri da requisição. */
	char *version; /* Versão da requisição. */
	char *hostname; /* Hostname da requisição. Mesmo ponteiro que está no header, está aqui por questão de praticidade. */
	char *body; /* Corpo da Requisição. */
	char *raw;  /* String da requisição completa, com todos os headers e corpo. */
	HeaderField *headers; /* Vetor de headers alocados dinâmicamente, usando o headerCount para controle.*/
	int headerCount;  /* Número de Headers. */
} HttpRequest;

/*
	Estrutura que armazena uma Resposta Http.
*/
typedef struct httpResponse{
	char *version; /* Versão Http da resposta. */
	short statusCode; /* Código de status da resposta http. */
	char *reasonPhrase; /* Reason phrase da resposta http. */
	char *body; /* Corpo da resposta. */
	char *raw; /* String da resposta completa, com todos os headers e corpo. */
	HeaderField *headers; /* Vetor de headers alocados dinâmicamente, usando o headerCount para controle.*/
	int headerCount; /* Número de Headers. */
} HttpResponse;


HttpResponse *httpSendRequest(HttpRequest *request);
HttpResponse *httpReceiveResponse(ThreadContext *context);
HttpRequest *httpReceiveRequest(ThreadContext *context);

void ResponsePrettyPrinter(HttpResponse *response);
void RequestPrettyPrinter(HttpRequest *request);

HeaderField *getHeaders(ThreadContext *context, char **raw, int *headerCount, int *req_size, int *has_body, int *body_size, char **hostname);
HeaderField *getLocalHeaders(char *resp, int *headerCount, int *req_size, int *has_body, int *body_size, char **hostname);
char *getBody(ThreadContext *context, char **raw, int *req_size, int body_size);
char *getLocalBody(char *resp, int *req_size, int body_size);

int FreeHttpResponse(HttpResponse *response);
int FreeHttpRequest(HttpRequest *request);

HttpResponse *httpParseResponse(char *response);



#endif