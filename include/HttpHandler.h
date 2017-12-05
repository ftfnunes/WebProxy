#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "Common.h"
#include "Log.h"
#include <errno.h>
#include "RequestValidator.h"
#include <time.h>
#include <gtk/gtk.h>

#ifndef HTTP_HANDLER

#define HTTP_HANDLER

#define BUFFER_SIZE 100000

extern int errno;

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
	HeaderField *headers; /* Vetor de headers alocados dinâmicamente, usando o headerCount para controle.*/
	int headerCount;  /* Número de Headers. */
	int bodySize;
} HttpRequest;

/*
	Estrutura que armazena uma Resposta Http.
*/
typedef struct httpResponse{
	char *version; /* Versão Http da resposta. */
	short statusCode; /* Código de status da resposta http. */
	char *reasonPhrase; /* Reason phrase da resposta http. */
	char *body; /* Corpo da resposta. */
	HeaderField *headers; /* Vetor de headers alocados dinâmicamente, usando o headerCount para controle.*/
	int headerCount; /* Número de Headers. */
	int bodySize;
} HttpResponse;

typedef struct guiStruct{
	GtkTextBuffer *buffer;
	HttpRequest *request;
} GuiStruct;

HeaderField *parseHeaderString(char *headers, int *headerCount);

int graphicInterface(HttpRequest *request);
void activate (GtkApplication* app, HttpRequest *request);
void sendBuffer (GuiStruct *guiStruct);

int HttpSendResponse(ThreadContext *context, HttpResponse *response);
HttpResponse *httpSendRequest(HttpRequest *request);

HttpResponse *httpReceiveResponse(ThreadContext *context);
HttpRequest *httpReceiveRequest(ThreadContext *context);

void ResponsePrettyPrinter(HttpResponse *response);
void RequestPrettyPrinter(HttpRequest *request);

HeaderField *getHeaders(ThreadContext *context, int *headerCount, int *has_body, int *body_size, char **hostname, int *is_chunked);
HeaderField *getLocalHeaders(char *resp, int *headerCount, int *req_size, int *has_body, int *body_size, char **hostname);
char *getBody(ThreadContext *context, int *body_size, int is_chunked);
char *getLocalBody(char *resp, int *req_size, int body_size);

int FreeHttpResponse(HttpResponse *response);
int FreeHttpRequest(HttpRequest *request);

HttpResponse *httpParseResponse(char *response, int length);

int getChunkedSize(ThreadContext *context, char **body, int *bodySize);

int sendRequest(ThreadContext *context, HttpRequest *request);

void freeResources(ThreadContext *context);

HttpResponse *blacklistResponseBuilder();

HttpResponse *deniedTermsResponseBuilder(ValidationResult *validation, int is_response);

char *GetHeadersString(HeaderField *headers, int headerCount);


#endif
