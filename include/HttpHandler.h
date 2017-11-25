#include <stdio.h>
#include <stdlib.h>

typedef enum methodType{
	GET,
	POST,
	DELETE,
	PUT,
	HEAD,
	OPTIONS,
	TRACE,
	CONNECT
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
	/* Vetor de headers alocados dinâmicamente, usando o headerCount para controle.*/
	HeaderField *headers;
	int headerCount;
} HttpRequest;

typedef struct httpResponse{
	char *version;
	short statusCode;
	char *reasonPhrase;
	char *body;
	/* Vetor de headers alocados dinâmicamente, usando o headerCount para controle.*/
	HeaderField *headers;
	int headerCount;
} HttpResponse;