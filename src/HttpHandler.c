#include "HttpHandler.h"
#include "RequestValidator.h"
#include "WebCache.h"
#include "Common.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "Log.h"

int HttpSendResponse(ThreadContext *context, HttpResponse *response){

	send(context->socket, response->raw, response->respSize*sizeof(char), 0);

}

HttpResponse *httpSendRequest(HttpRequest *request){
	HttpResponse *response = NULL;
	ThreadContext context;
	int sockfd;  
	struct addrinfo hints, *servinfo = NULL, *p = NULL;
	int rv;
	char buffer[4000];

	bzero(buffer, 4000);

	logSuccess("Request recebida em 'httpSendRequest'.");
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // use AF_INET6 to force IPv6
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(request->hostname, "http", &hints, &servinfo)) != 0) {
		logError("Erro ao obter informacoes de DNS.");
	    exit(1);
	}

	/* O loop passa pelos endereços recebidos e busca um que não seja nulo e que seja possível conectar. */
	for(p = servinfo; p != NULL; p = p->ai_next) {
    	if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			logError("Erro ao criar o socket.");
    	    continue;
    	}
	
	    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
	        printf(buffer,"Erro ao tentar conectar com o servidor: %s", request->hostname);
			logError(buffer);
			bzero(buffer, 4000);
	        close(sockfd);
	        continue;
	    }
	
    	break; 
	}

	if( p == NULL){
		sprintf(buffer,"Nao foi possivel conectar ao servidor: %s", request->hostname);
		logError(buffer);
		bzero(buffer, 4000);
		exit(1);
	} else{
		sprintf(buffer,"Conexao estabelecida com o host: %s", request->hostname);
		logSuccess(buffer);
		bzero(buffer, 4000);
	}

	if(send(sockfd, request->raw, request->reqSize*sizeof(char), 0) < 0){
		sprintf(buffer,"Erro ao enviar requisição para o host: %s", request->hostname);
		logError(buffer);
		bzero(buffer, 4000);
		freeaddrinfo(servinfo);
		close(sockfd);
		exit(1);
	} else {
		sprintf(buffer,"Requisição enviada com sucesso para o host: %s", request->hostname);
		logSuccess(buffer);
		bzero(buffer, 4000);
	}

	context.socket = sockfd;
	context.sockAddr = NULL;

	response = httpReceiveResponse(&context);

	if(response == NULL){
		sprintf(buffer,"Erro ao realizar o parse da response do host: %s", request->hostname);
		logError(buffer);
		bzero(buffer, 4000);
	} else{
		sprintf(buffer,"Response recebida e parse realizado do host: %s", request->hostname);
		logSuccess(buffer);
		bzero(buffer, 4000);
	}

	freeaddrinfo(servinfo);
	close(sockfd);
	return response;
}


/*
	Função que libera os recursos alocados por uma struct HttRequest e também a própria requisição.
	A função sempre verifica se o ponteiro está nulo antes de liberar, para não acontecer falha de segmentação (SEGFAULT). 
*/

int FreeHttpRequest(HttpRequest *request){
	int i;

	if(request != NULL){
		if(request->method != NULL){
			free(request->method);
		}
		if(request->uri != NULL){
			free(request->uri);
		}
		if(request->version != NULL){
			free(request->version);
		}
		if(request->body != NULL){
			free(request->body);
		}
		if(request->raw != NULL){
			free(request->raw);
		}

		for(i = 0; i < request->headerCount; ++i){
			if(request->headers[i].name != NULL){
				free(request->headers[i].name);
			}
			if(request->headers[i].value != NULL){
				free(request->headers[i].value);
			}
		}
	}
	

	return 1;
}


/*
	Função que libera os recursos alocados por uma struct HttResponse e também a própria resposta.
	A função sempre verifica se o ponteiro está nulo antes de liberar, para não acontecer falha de segmentação (SEGFAULT). 
*/
int FreeHttpResponse(HttpResponse *response){
	int i;

	if(response != NULL){
		if(response->reasonPhrase != NULL){
			free(response->reasonPhrase);
		}
		if(response->version != NULL){
			free(response->version);
		}
		if(response->body != NULL){
			free(response->body);
		}
		if(response->raw != NULL){
			free(response->raw);
		}

		for(i = 0; i < response->headerCount; ++i){
			if(response->headers[i].name != NULL){
				free(response->headers[i].name);
			}
			if(response->headers[i].value != NULL){
				free(response->headers[i].value);
			}
		}
	}
	

	return 1;
}

/**
	
	"httpReceiveResponse": A partir do socket presente no context, a função recebe uma response Http completa e realiza seu devido parse. A response é mapeada para uma struct do tipo HttpResponse, alocada dinâmicamente.

*/
HttpResponse *httpReceiveResponse(ThreadContext *context){
	int length, size, req_size, has_body = 0;

	/* Buffer de 3000, pois foi pesquisado que URL's com mais de 2000 caracteres podem não funcionar em todos as conexões cliente-servidor.*/
	char buffer[3000], buff;
	HttpResponse *response = (HttpResponse *)malloc(sizeof(HttpResponse));

	/* Todos os ponteiros são inicializados com NULL e o status code inicial é de -1 para mostrar que ele não foi recebido ainda. */
	response->version = NULL;
	response->statusCode = -1;
	response->reasonPhrase = NULL;
	response->body = NULL;
	response->raw = NULL;
	response->headers = NULL;
	response->headerCount = 0;
	response->bodySize = 0;

	req_size = 0;
	size = 0;

	/* A função bzero(void *ptr, int size) escreve 0 em size posições do vetor ptr. */
	bzero(buffer, 3000);


	/* 

		Loop que recebe a primeira linha da resposta.
		A leitura é feita caractere a caractere do Socket.

		São lidos caracteres até encontrar o primeiro espaço. O que foi lido até aí é a versão, que é copiada para memória alocada para a struct de resposta.

		Novamente são lidos caracteres até encontrar um novo espaço. O que foi lido até este ponto foi o código de status, string que é transformada em um short para ser armazenado npo struct.

		O próximo campo a ser preenchido é o de reason phrase. Como este campo pode conter espaçoes, primeiro é verificado se o status code e a versão já foram adquirdos, para que sejam lidos caracteres até que seja encontrado um caractere '\r'. Só aí é alocado espaço para o reason phrase e então a struct recebe o ponteiro com os dados alocados.

	*/
	while(1){
		/* Leitura byte a byte do Socket. */
		length = recv(context->socket, &buff, 1, 0);
		if(length < 1){
			printf("Error on receiving data from socket. Size < 0.\n");
			exit(1);
		}

		/* É armazenado em Raw a requisição completa. */
		response->raw = (char *)realloc(response->raw, (req_size+1)*sizeof(char));
		response->raw[req_size] = buff;
		++req_size;

		/* 

			Trigger de saída do loop. Quando o primeiro '\n' é encontrado, a pirmeira linha da requisição foi totalmente adquirida.

		*/
		if(buff == '\n'){
			bzero(buffer, 3000);
			break;
		}


		/*
			
			Caso o caractere lido for ' ' ou '\r', significa que é o momento de armazenar algum dos campos da resposta. Primeiro é adquirida a versão da resposta e em seguida o status code.

			A reason phrase só é adquirida quando o caractere encontrado for '\r', porque ela pode conter espaços e tem fim no término da primeira linha.

			Caso um caractere diferente desse for encontrado ele é armazenado no buffer.

		*/
		if(buff == ' ' || buff == '\r'){
			if(response->version == NULL){
				response->version = (char *)malloc((size+1)*sizeof(char));
				strcpy(response->version, buffer);
				response->version[size] = '\0';
				//printf("RESPONSE - Version %s\n", response->version);
				bzero(buffer, 3000);
				size = 0;
			} else if(response->statusCode == -1){
				response->statusCode = (short)atoi(buffer);
				//printf("RESPONSE - Code: %d\n", response->statusCode);
				bzero(buffer, 3000);
				size = 0;
			} else if(buff == '\r'){
				response->reasonPhrase = (char *)malloc((size+1)*sizeof(char));
				strcpy(response->reasonPhrase, buffer);
				response->reasonPhrase[size] = '\0';
				//printf("RESPONSE - Phrase: %s\n", response->reasonPhrase);
				bzero(buffer, 3000);
				size = 0;
			}

		} else{
			buffer[size] = buff;
			++size;
		}
	}


	/* Após a primeira linha da resposta ser adquirida, os headers são pegos. Na resposta o último parâmetro é NULL pelo fato de não ser armazenado na struct o Hostname. */
	response->headers = getHeaders(context, &(response->raw), &(response->headerCount), &req_size, &has_body, &response->bodySize, NULL);
	
	/* Caso a resposta tenha corpo, ele é adquirido com a função getBody. */
	if( has_body == 1 ){
		response->body = getBody(context, &(response->raw), &req_size, response->bodySize);
	}

	response->raw = (char *)realloc(response->raw, (req_size+1)*sizeof(char));
	response->raw[req_size] = '\0';
	++req_size;

	//ResponsePrettyPrinter(response);

	response->respSize = req_size;
	return response;
}


/**
	
	"httpReceiveRequest": A partir do socket presente no context, a função recebe uma request Http completa e realiza seu devido parse. A request é mapeada para uma struct do tipo HttpRequest, alocada dinâmicamente.

*/

HttpRequest *httpReceiveRequest(ThreadContext *context){
	int length, size, req_size, has_body = 0;

	/* Buffer de 3000, pois foi pesquisado que URL's com mais de 2000 caracteres podem não funcionar em todos as conexões cliente-servidor.*/
	char buffer[3000], buff;
	HttpRequest *request = (HttpRequest *)malloc(sizeof(HttpRequest));

	/* 

		Todos os ponteiros são inicializados com NULL. 

	*/
	request->method = NULL;
	request->uri = NULL;
	request->version = NULL;
	request->hostname = NULL;
	request->body = NULL;
	request->raw = NULL;
	request->headerCount = 0;
	request->headers = NULL;
	request->bodySize = 0;

	req_size = 0;
	size = 0;
	/* A função bzero(void *ptr, int size) escreve 0 em size posições do vetor ptr. */
	bzero(buffer, 3000);

	/* 

		Loop que recebe a primeira linha da requisição.
		A leitura é feita caractere a caractere do Socket.

		São lidos caracteres até encontrar o primeiro espaço. O que foi lido até aí foi o método da requisição, que é copiada para memória alocada para a struct de request.

		Novamente são lidos caracteres até encontrar um novo espaço. O que foi lido até este ponto foi a URI, então espaço é alocado para ela na struct de resposta.

		Então, finalmente é lida a versão Http da requisição, que também é armazenada na struct na forma de ponteiro.

	*/

	while(1){
		/* Leitura byte a byte do Socket. */
		length = recv(context->socket, &buff, 1, 0);
		if(length < 1){
			printf("Error on receiving data from socket. Size < 0.\n");
			exit(1);
		}

		/* É armazenado em Raw a requisição completa. */
		request->raw = (char *)realloc(request->raw, (req_size+1)*sizeof(char));
		request->raw[req_size] = buff;
		++req_size;


		/* 

			Trigger de saída do loop. Quando o primeiro '\n' é encontrado, a pirmeira linha da requisição foi totalmente adquirida. O buffer é zerado e há a saída do loop.

		*/
		if(buff == '\n'){
			bzero(buffer, 3000);
			break;
		}


		/*
			
			Caso o caractere lido for ' ' ou '\r', significa que é o momento de armazenar algum dos campos da resposta. Primeiro é adquirido o método da requisição, em seguida a URI da requisição e fianlmente a versão do Http.

			A memória para armazenar cada uma dessas informações é alocada dinâmicamente e as strings são copiadas do buffer.

		*/
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

	/* A partir do método da requisição, é armazenado seu tipo com o enum 'MethodTypeMethodType'. */
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


	/* Após a primeira linha da requisição ser adquirida, os headers são pegos. */
	request->headers = getHeaders(context, &(request->raw), &(request->headerCount), &req_size, &has_body, &request->bodySize, &request->hostname);
	
	/* Caso a requisição tenha corpo, ele é adquirido com a função getBody. */
	if( has_body == 1 ){
		request->body = getBody(context, &(request->raw), &req_size, request->bodySize);
	}

	request->raw = (char *)realloc(request->raw, (req_size+1)*sizeof(char));
	request->raw[req_size] = '\0';
	++req_size;
	//RequestPrettyPrinter(request);

	request->reqSize = req_size;
	return request;
}

/*  

	Função que a partir de um socket do contexto, lê todos os headers de uma requisição ou resposta.

	O ponteiro para o raw da requisição, o contador de headers, o tamanho da requisição, a flag has_body (que é ativada se a requisição/resposta tem corpo), a variável body_size (que armazena o tamanho do corpo da requisição/resposta) e o ponteiro de hostname	são passados por referência porque seus valores podem ser modificados dentro da função.

	O loop principal da função só termina quando são encontrados em sequência os caracters "\r\n\r\n", sequência que simboliza o final dos headers.

	A estrutura que armazena um header é composta por um "name", que simboliza o campo do header, e "value", que é valor do campo, que inclusive pode conter espaços.

*/
HeaderField *getHeaders(ThreadContext *context, char **raw, int *headerCount, int *req_size, int *has_body, int *body_size, char **hostname){
	int length = 0, found_linebreak = 0, size = 0, found_name = 0;
	char buff, buffer[3000], *value = NULL, *name = NULL;
	HeaderField *headers = NULL;

	while(1){
		length = recv(context->socket, &buff, 1, 0);
		if(length < 1){
			printf("Error on receiving data from socket. Size < 0.\n");
			exit(1);
		}

		(*raw) = (char *)realloc((*raw), ((*req_size)+1)*sizeof(char));
		(*raw)[(*req_size)] = buff;
		(*req_size) = (*req_size) + 1;

		/* 
			Verificações que detectam a sequência de saída do loop. Só há a saída quando a sequência lida é "\r\n\r\n". 
		*/
		if(found_linebreak == 1 && buff == '\r'){
			++found_linebreak;
			continue;
		} else if(found_linebreak == 2 && buff == '\n'){
			break;
		} else {
			found_linebreak = 0;
		}

		/*
			Quando um '\n' é encontrado significa que o "name" e "value" encontrados devem ser aramazenados na lista de headers.
		*/
		if(buff == '\n'){
			//printf("Push headers.\n");
			headers = (HeaderField *)realloc(headers, ((*headerCount)+1)*sizeof(HeaderField));
			//printf("Deu push.\n");
			//printf("Antes - Name: %s\nValue:%s\n\n", name, value);
			headers[*headerCount].name = name;
			headers[*headerCount].value = value;
			//printf("Depois - Name: %s\nValue:%s\n\n", headers[*headerCount].name, headers[*headerCount].value);
			++(*headerCount);
			
			found_linebreak = 1;
			size = 0;
			bzero(buffer, 3000);
			continue;
		}


		/* 

			Se o caractere encontrado é um espaço e o "name" do header não foi definido, é alocada memória para ele e então seu valor é copiado do buffer que o armazenado, sendo este buffer zerado em seguida. O caractere ':' não é armazenado no "name". 

			Se o "name" do header foi encontrado e o caractere atual é um '\r', todo o conteúdo de "value" já está atualmente no buffer, então espaço para o "value" é alocado dinâmicamente e seu valor é copiado do buffer, que é zerado em seguida.

			Caso as condições acima não sejam satisfeitas, o caractere atual é armazenado no buffer.

		*/
		if(buff == ' ' && found_name == 0){
			name = (char *)malloc(size*sizeof(char));
			strcpy(name, buffer);
			name[size-1] = '\0';
			found_name = 1;

			
			bzero(buffer, 3000);
			size = 0;
		} else if(buff == '\r' && found_name == 1){
			value = (char *)malloc((size+1)*sizeof(char));
			strcpy(value, buffer);
			value[size] = '\0';

			/* 
				Se o campo atual for "Content-Length", seu valor significa que a requisição tem corpo e seu valor é o tamanho do corpo em questão, por isso as variáveis "body_size" e "has_body" são atualizadas.
			*/
			if(strcmp(name, "Content-Length") == 0){
				*body_size = atoi(value);
				if((*body_size) > 0){
					*has_body = 1;
				}				
			}

			/*

				Se o "name" atual for "Host" e o ponteiro passado na função não for NULL (é uma requisição em questão), o ponteiro de hostname também recebe a referência para o "value".

			*/
			if(strcmp(name, "Host") == 0 && hostname != NULL){
				*hostname = value;
			}
			
			found_name = 0;
			bzero(buffer, 3000);
			size = 0;
		} else {
			buffer[size] = buff;
			++size;
		}
	}

	/* Após adquirir todos os headers, a função retorna um ponteiro de HeaderField alocado dinâmicamente. */
	return headers;
}

/*
	
	A partir de um socket armazenado num contexto, são lidos "body_size" bytes do Socket, bytes esses que compõe o corpo da requisção. O corpo também é armazenado no Raw da requisição.

	As variáveis raw e req_size são passadas por referência pois tem seu valor modificado dentro da função.

*/
char *getBody(ThreadContext *context, char **raw, int *req_size, int body_size){
	int length, size;
	char *body;

	/* É alocada memória para o corpo da requisição. */
	body = (char *)malloc((body_size+1)*sizeof(char));

	/* A leitura é feita para o espaço criado para o corpo. */
	length = recv(context->socket, body, body_size, MSG_WAITALL);
	if(length < 0){
		printf("Error on receiving data from socket. Size < 0.\n");
		exit(1);
	}
	
	body[body_size] = '\0';
	

	/*
	
		O espaço de aramazenamento do raw da requisição é aumentado para caber os bytes do corpo da requisição. A cópia é realizada byte a byte.
	
	*/
	(*raw) = (char *)realloc((*raw), ((*req_size)+body_size+1)*sizeof(char));
	
	for (size = 0; size < body_size; ++size){
		(*raw)[(*req_size)] = body[size];
		printf("Char: %c\n", (*raw)[(*req_size)]);
		++(*req_size);
	}
	(*raw)[(*req_size)] = '\0';
	++(*req_size);


	/* O ponteiro é retornado com os dados do corpo. */
	return body;
}


/**
	
	"httpParseResponse": A partir do string Raw de uma response Http completa e realiza seu devido parse. A response é mapeada para uma struct do tipo HttpResponse, alocada dinâmicamente.

*/
HttpResponse *httpParseResponse(char *resp) {
	int size, body_size = 0, req_size, has_body = 0;

	/* Buffer de 3000, pois foi pesquisado que URL's com mais de 2000 caracteres podem não funcionar em todos as conexões cliente-servidor.*/
	char buffer[3000], buff;
	HttpResponse *response = (HttpResponse *)malloc(sizeof(HttpResponse));

	/* Todos os ponteiros são inicializados com NULL e o status code inicial é de -1 para mostrar que ele não foi recebido ainda. */
	response->version = NULL;
	response->statusCode = -1;
	response->reasonPhrase = NULL;
	response->body = NULL;
	response->headers = NULL;
	response->headerCount = 0;


	req_size = 0;
	size = 0;
	bzero(buffer, 3000);

	/* 
		O campo de Raw da resposta recebe o ponteiro da requisição raw inteira.
	*/
	response->raw = resp;


	/* 

		Loop que recebe a primeira linha da resposta.
		A leitura é feita caractere a caractere da string Raw.

		São lidos caracteres até encontrar o primeiro espaço. O que foi lido até aí é a versão, que é copiada para memória alocada para a struct de resposta.

		Novamente são lidos caracteres até encontrar um novo espaço. O que foi lido até este ponto foi o código de status, string que é transformada em um short para ser armazenado npo struct.

		O próximo campo a ser preenchido é o de reason phrase. Como este campo pode conter espaçoes, primeiro é verificado se o status code e a versão já foram adquirdos, para que sejam lidos caracteres até que seja encontrado um caractere '\r'. Só aí é alocado espaço para o reason phrase e então a struct recebe o ponteiro com os dados alocados.

	*/
	while(1){

		/* Leitura é feita da string recebida. */
		buff = resp[req_size];
		++req_size;

		/* 

			Trigger de saída do loop. Quando o primeiro '\n' é encontrado, a pirmeira linha da requisição foi totalmente adquirida.

		*/
		if(buff == '\n'){
			bzero(buffer, 3000);
			break;
		}


		/*
			
			Caso o caractere lido for ' ' ou '\r', significa que é o momento de armazenar algum dos campos da resposta. Primeiro é adquirida a versão da resposta e em seguida o status code.

			A reason phrase só é adquirida quando o caractere encontrado for '\r', porque ela pode conter espaços e tem fim no término da primeira linha.

			Caso um caractere diferente desse for encontrado ele é armazenado no buffer.

		*/
		if(buff == ' ' || buff == '\r'){
			if(response->version == NULL){
				response->version = (char *)malloc((size+1)*sizeof(char));
				strcpy(response->version, buffer);
				response->version[size] = '\0';
				bzero(buffer, 3000);
				size = 0;
			} else if(response->statusCode == -1){
				response->statusCode = (short)atoi(buffer);
				bzero(buffer, 3000);
				size = 0;
			} else if(buff == '\r'){
				response->reasonPhrase = (char *)malloc((size+1)*sizeof(char));
				strcpy(response->reasonPhrase, buffer);
				response->reasonPhrase[size] = '\0';
			}

		} else{
			buffer[size] = buff;
			++size;
		}
	}

	/* 
		Após a primeira linha da resposta ser adquirida, os headers são pegos. Na resposta o último parâmetro é NULL pelo fato de não ser armazenado na struct o Hostname. 
	*/
	response->headers = getLocalHeaders(resp, &(response->headerCount), &req_size, &has_body, &body_size, NULL);
	

	/* 
		Caso a resposta tenha corpo, ele é adquirido com a função getLocalBody, função que adquire o corpo a partir do Raw de uma requisição. 
	*/
	if( has_body == 1 ){
		response->body = getLocalBody(resp, &req_size, body_size);
	}


	//ResponsePrettyPrinter(response);

	response->respSize = req_size;
	return response;
}


/*  

	Função que a partir de uma string com uma requisição raw, lê todos os headers de uma requisição ou resposta.

	O contador de headers, o tamanho da requisição, a flag has_body (que é ativada se a requisição/resposta tem corpo), a variável body_size (que armazena o tamanho do corpo da requisição/resposta) e o ponteiro de hostname	são passados por referência porque seus valores podem ser modificados dentro da função.

	O loop principal da função só termina quando são encontrados em sequência os caracters "\r\n\r\n", sequência que simboliza o final dos headers.

	A estrutura que armazena um header é composta por um "name", que simboliza o campo do header, e "value", que é valor do campo, que inclusive pode conter espaços.

*/
HeaderField *getLocalHeaders(char *resp, int *headerCount, int *req_size, int *has_body, int *body_size, char **hostname){
	int found_linebreak = 0, size = 0, found_name = 0;
	char buff = '\0', buffer[3000], *value = NULL, *name = NULL;
	HeaderField *headers = NULL;


	while(1){
		buff = resp[(*req_size)];
		(*req_size) = (*req_size) + 1;


		/* 
			Verificações que detectam a sequência de saída do loop. Só há a saída quando a sequência lida é "\r\n\r\n". 
		*/
		if(found_linebreak == 1 && buff == '\r'){
			++found_linebreak;
			continue;
		} else if(found_linebreak == 2 && buff == '\n'){
			break;
		} else {
			found_linebreak = 0;
		}


		/*
			Quando um '\n' é encontrado significa que o "name" e "value" encontrados devem ser armazenados na lista de headers.
		*/
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


		/* 

			Se o caractere encontrado é um espaço e o "name" do header não foi definido, é alocada memória para ele e então seu valor é copiado do buffer que o armazenado, sendo este buffer zerado em seguida. O caractere ':' não é armazenado no "name". 

			Se o "name" do header foi encontrado e o caractere atual é um '\r', todo o conteúdo de "value" já está atualmente no buffer, então espaço para o "value" é alocado dinâmicamente e seu valor é copiado do buffer, que é zerado em seguida.

			Caso as condições acima não sejam satisfeitas, o caractere atual é armazenado no buffer.

		*/
		if(buff == ' ' && found_name == 0){
			name = (char *)malloc(size*sizeof(char));
			strcpy(name, buffer);
			name[size-1] = '\0';
			found_name = 1;

			
			bzero(buffer, 3000);
			size = 0;
		} else if(buff == '\r' && found_name == 1){
			value = (char *)malloc((size+1)*sizeof(char));
			strcpy(value, buffer);
			value[size] = '\0';

			/* 
				Se o campo atual for "Content-Length", seu valor significa que a requisição tem corpo e seu valor é o tamanho do corpo em questão, por isso as variáveis "body_size" e "has_body" são atualizadas.
			*/
			if(strcmp(name, "Content-Length") == 0){
				*body_size = atoi(value);
				if((*body_size) > 0){
					*has_body = 1;
				}				
			}

			/*

				Se o "name" atual for "Host" e o ponteiro passado na função não for NULL (é uma requisição em questão), o ponteiro de hostname também recebe a referência para o "value".

			*/
			if(strcmp(name, "Host") == 0 && hostname != NULL){
				*hostname = value;
			}
			
			found_name = 0;
			bzero(buffer, 3000);
			size = 0;
		} else {
			buffer[size] = buff;
			++size;
		}
	}

	/* Após adquirir todos os headers, a função retorna um ponteiro de HeaderField alocado dinâmicamente. */
	return headers;
}


/*
	O corpo da requisição é lido do Raw da string. A posição inicial do corpo é dada por 'req_size'.
*/
char *getLocalBody(char *resp, int *req_size, int body_size){
	char *body, *readings;

	body = (char *)malloc((body_size+1)*sizeof(char));

	readings = strcpy(body, &(resp[(*req_size)]));
	if(readings == NULL){
		printf("Leitura não para body não realizado.\n");
		exit(1);
	}
	
	body[body_size] = '\0';

	(*req_size) += body_size + 1; 

	printf("Local body: %s\n\n", body);
	

	return body;
}


/*
	Função que imprime todos os campos e headers de uma HttpResponse.
*/
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


/*
	Função que imprime todos os campos e headers de uma HttpRequest.
*/
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