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
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "RequestValidator.h"
#include <time.h>
/*
	A partir de uma string inteira com todos os headers, é retornado um objeto com um array de objetos de HeaderField alocados dinâmicamente.
*/
HeaderField *parseHeaderString(char *headers, int *headerCount, char **hostname){
	int req_size = 0;
	int size = strlen(headers), i = 0, j = 0, has_body = 0, body_size = 0;
	HeaderField *head;
	char buffer[1000000], *strbuff;

	bzero(buffer, 1000000);
	(*headerCount) = 0;
	for(i = 0; i < size; ++i){
		if(headers[i] == '\n' && i > 0){
			if(headers[i-1] == '\r'){
				buffer[j] = headers[i];
				++j;
			} else{
				buffer[j] = '\r';
				++j;
				buffer[j] = '\n';
				++j;
			}
			++(*headerCount);
		} else{
			buffer[j] = headers[i];
			++j;
		}
	}

	buffer[j] = '\r';
	++j;
	buffer[j] = '\n';
	++j;

	strbuff = (char *)calloc(j+1, sizeof(char));
	for (i = 0; i < j; i++) {
		strbuff[i] = buffer[i];
	}

	(*headerCount) = 0;

	head = getLocalHeaders(strbuff, headerCount, &req_size, &has_body, &body_size, hostname);

	return head;
}

/*
	Função de callback da janela de edição de Headers. A partir dos headers modificados, gera nova lista de headers, alocados dinâmicamente.
*/
void sendBuffer (GuiStruct *guiStruct){
	char *str;
	int i;

	for(i = 0; i < guiStruct->request->headerCount; i++){
		free(guiStruct->request->headers[i].name);
		free(guiStruct->request->headers[i].value);
	}
	free(guiStruct->request->headers);

	GtkTextIter start, end;
	gtk_text_buffer_get_start_iter(guiStruct->buffer, &start);
	gtk_text_buffer_get_end_iter(guiStruct->buffer, &end);
	str = gtk_text_buffer_get_text(guiStruct->buffer, &start, &end, TRUE);

	guiStruct->request->headers = parseHeaderString(str, &(guiStruct->request->headerCount), &(guiStruct->request->hostname));

  free(guiStruct);
}

/*
	Função que aloca recursos e configura a janela de edição de requisições.
*/
void activate (GtkApplication* app, HttpRequest *request){
	GtkWidget *window;
	GtkWidget *view;
	GtkTextBuffer *buffer;
	GtkWidget *button;
  	GtkWidget *button_box;
  	GtkWidget *box;
  	GuiStruct *guiStruct;
  	char *string = GetHeadersString(request->headers, request->headerCount);

	window = gtk_application_window_new (app);
  	gtk_window_set_title (GTK_WINDOW (window), "Inspect");
  	gtk_window_set_default_size (GTK_WINDOW (window), 400, 400);

  	box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 50);
	gtk_container_add (GTK_CONTAINER (window), box);

  	view = gtk_text_view_new ();
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	gtk_text_buffer_set_text (buffer, string, strlen(string));
	gtk_container_add (GTK_CONTAINER (box), view);

	free(string);

	button_box = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
  	gtk_container_add (GTK_CONTAINER (box), button_box);

  	guiStruct = (GuiStruct *)malloc(sizeof(GuiStruct));
  	guiStruct->buffer = buffer;
  	guiStruct->request = request;

  	button = gtk_button_new_with_label ("Send");
  	g_signal_connect_swapped (button, "clicked", G_CALLBACK (sendBuffer), guiStruct);
  	g_signal_connect_swapped (button, "clicked", G_CALLBACK (gtk_widget_destroy), window);
  	gtk_container_add (GTK_CONTAINER (button_box), button);

	gtk_widget_show_all (window);
}

/*
	Função chamada para ativação da janela de edição de headers. Ela chama a função "activate".
*/
int graphicInterface(HttpRequest *request){
  	GtkApplication *app;
  	int status;

  	app = gtk_application_new ("org.bff", G_APPLICATION_FLAGS_NONE);
  	g_signal_connect (app, "activate", G_CALLBACK (activate), request);
  	status = g_application_run (G_APPLICATION (app), 0, NULL);
  	g_object_unref (app);

  	return status;
}

/*
	Função que libera recursos de context.
*/
void freeResources(ThreadContext *context) {
	close(context->socket);
	free(context->sockAddr);
	free(context);
}

/*
	Função que a partir de um contexto, envia no socket a response recebida como parâmetro.
*/
int HttpSendResponse(ThreadContext *context, HttpResponse *response){
	char buffer[BUFFER_SIZE], buff[500];
	int i, length, foundConnection = FALSE;

	//logSuccess("Entrou no HttpSendResponse.");
	//logSuccess("Alocou memoria para o buffer.");

	bzero(buffer, BUFFER_SIZE);

	sprintf(buffer, "%s %d %s\r\n", response->version, response->statusCode, response->reasonPhrase);
	//logSuccess("Escreveu no buffer.");
	length = send(context->socket, buffer, strlen(buffer)*sizeof(char), MSG_NOSIGNAL);
	//bzero(buff, 500);
	//sprintf(buff, "Envou dados. - Length: %d - Strlen: %ld.", length, strlen(buffer));
	//logSuccess(buff);

	if(length != (strlen(buffer))){
		bzero(buff, 500);
		sprintf(buff, "Erro ao enviar primeira linha da response. Enviados: %d - Length: %ld.", length, strlen(buffer));
		logError(buff);
		freeResources(context);
		pthread_exit(NULL);
	}


	for (i = 0; i < response->headerCount; ++i){
		bzero(buffer, BUFFER_SIZE);

		if(strcmp(response->headers[i].name, "Keep-Alive") == 0){
			continue;
		}

		if(strcmp(response->headers[i].name, "Connection") == 0) {
			sprintf(buffer, "%s: close\r\n", response->headers[i].name);
			length = send(context->socket, buffer, strlen(buffer)*sizeof(char), MSG_NOSIGNAL);
			foundConnection = TRUE;
		} else{
			sprintf(buffer, "%s: %s\r\n", response->headers[i].name, response->headers[i].value);
			length = send(context->socket, buffer, strlen(buffer)*sizeof(char), MSG_NOSIGNAL);
		}

		if(length != (strlen(buffer))){
			bzero(buff, 500);
			sprintf(buff, "Erro ao enviar header da response. Enviados: %d - Length: %ld. Name: %s - Value:%s", length, strlen(buffer), response->headers[i].name, response->headers[i].value);
			logError(buff);
			freeResources(context);
			pthread_exit(NULL);
		}
	}

	if (!foundConnection) {
		bzero(buffer, BUFFER_SIZE);
		sprintf(buffer, "Connection: close\r\n");
		length = send(context->socket, buffer, strlen(buffer)*sizeof(char), MSG_NOSIGNAL);
		if(length != (strlen(buffer))){
			bzero(buff, 500);
			sprintf(buff, "Erro ao enviar header da response. Enviados: %d - Length: %ld. Name: %s - Value:%s", length, strlen(buffer), response->headers[i].name, response->headers[i].value);
			logError(buff);
			freeResources(context);
			pthread_exit(NULL);
		}
	}

	if ((length = send(context->socket, "\r\n", 2*sizeof(char), MSG_NOSIGNAL)) < 2) {
		bzero(buff, 500);
		sprintf(buff, "Not sent all bytes. Bytes sent: %d. Bytes needed to send: %d.", length, 2);
		logError(buff);
		freeResources(context);
		pthread_exit(NULL);
	}
	if(response->body != NULL && response->bodySize > 0){

		length = send(context->socket, response->body, response->bodySize*sizeof(char), MSG_NOSIGNAL);
		if(length != (response->bodySize)*sizeof(char)){
			bzero(buff, 500);
			sprintf(buff, "Not sent all bytes. Bytes sent: %d. Bytes needed to send: %d.", length, response->bodySize);
			logError(buff);
			freeResources(context);
			pthread_exit(NULL);
		}
	}

	return 0;
}

/*
	Função que a partir de um contexto, envia no socket a request recebida como parâmetro.
*/
int sendRequest(ThreadContext *context, HttpRequest *request){
	char buffer[BUFFER_SIZE], buff[BUFFER_SIZE];
	int i, length, foundConnection = FALSE;

	//logSuccess("Entrou no sendRequest.");
	//logSuccess("Alocou memoria para o buffer.");

	bzero(buffer, BUFFER_SIZE);

	sprintf(buffer, "%s %s %s\r\n", request->method, request->uri, request->version);
	//logSuccess("Escreveu no buffer.");
	length = send(context->socket, buffer, strlen(buffer)*sizeof(char), MSG_NOSIGNAL);
	bzero(buff, BUFFER_SIZE);
	//printf("Envou dados. - Length: %d - Strlen: %ld.", length, strlen(buffer));
	//logSuccess(buff);

	if(length != (strlen(buffer))){
		bzero(buff, BUFFER_SIZE);
		sprintf(buff, "Erro ao enviar primeira linha da request. Enviados: %d - Length: %ld.", length, strlen(buffer));
		logError(buff);
		freeResources(context);
		pthread_exit(NULL);
	}


	for (i = 0; i < request->headerCount; ++i){
		bzero(buffer, BUFFER_SIZE);
		if(strcmp(request->headers[i].name, "Keep-Alive") == 0){
			continue;
		}

		if(strcmp(request->headers[i].name, "Connection") == 0) {
			sprintf(buffer, "%s: close\r\n", request->headers[i].name);
			length = send(context->socket, buffer, strlen(buffer)*sizeof(char), MSG_NOSIGNAL);
			foundConnection = TRUE;
		} else{
			sprintf(buffer, "%s: %s\r\n", request->headers[i].name, request->headers[i].value);
			length = send(context->socket, buffer, strlen(buffer)*sizeof(char), MSG_NOSIGNAL);
		}

		if(length != (strlen(buffer))){
			bzero(buff, BUFFER_SIZE);
			sprintf(buff, "Erro ao enviar header da response. Enviados: %d - Length: %ld. Name: %s - Value:%s", length, strlen(buffer), request->headers[i].name, request->headers[i].value);
			logError(buff);
			freeResources(context);
			pthread_exit(NULL);
		}
	}

	if (!foundConnection) {
		bzero(buffer, BUFFER_SIZE);
		sprintf(buffer, "Connection: close\r\n");
		length = send(context->socket, buffer, strlen(buffer)*sizeof(char), MSG_NOSIGNAL);
		if(length != (strlen(buffer))){
			bzero(buff, BUFFER_SIZE);
			sprintf(buff, "Erro ao enviar header da response. Enviados: %d - Length: %ld.", length, strlen(buffer));
			logError(buff);
			freeResources(context);
			pthread_exit(NULL);
		}
	}

	if ((length = send(context->socket, "\r\n", 2*sizeof(char), MSG_NOSIGNAL)) < 2) {
		bzero(buff, 500);
		sprintf(buff, "Not sent all bytes. Bytes sent: %d. Bytes needed to send: %d.", length, 2);
		logError(buff);
		freeResources(context);
		pthread_exit(NULL);
	}
	if(request->body != NULL && request->bodySize > 0){

		length = send(context->socket, request->body, request->bodySize*sizeof(char), MSG_NOSIGNAL);
		if(length != (request->bodySize)*sizeof(char)){
			bzero(buff, 500);
			sprintf(buff, "Not sent all bytes. Bytes sent: %d. Bytes needed to send: %d.", length, request->bodySize);
			logError(buff);
			freeResources(context);
			pthread_exit(NULL);
		}
	}

	return 0;
}

/*
	A partir de uma request recebida, busca o destino original da request com o hostname, envia a request, faz o parse da response do servidor e retorna a response.
*/
HttpResponse *httpSendRequest(HttpRequest *request){
	HttpResponse *response = NULL;
	ThreadContext context;
	int sockfd;
	struct addrinfo hints, *servinfo = NULL, *p = NULL;
	int rv;
	char buffer[4000];

	bzero(buffer, 4000);

	//logSuccess("Request recebida em 'httpSendRequest'.");
	memset(&hints, 0, sizeof(hints));
	//logSuccess("Memset realizado em 'httpSendRequest'.");
	hints.ai_family = AF_UNSPEC; // use AF_INET6 to force IPv6
	hints.ai_socktype = SOCK_STREAM;

	//printf("\n\n\n\n\n\n\n\n\nHost: %s\n", request->hostname);
	if ((rv = getaddrinfo(request->hostname, "http", &hints, &servinfo)) != 0) {
		logSuccess("Erro ao obter informacoes de DNS.");
	    pthread_exit(NULL);
	}

	/* O loop passa pelos endereços recebidos e busca um que não seja nulo e que seja possível conectar. */
	for(p = servinfo; p != NULL; p = p->ai_next) {
		//logSuccess("Tentou criar o socket.");
    	if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			logSuccess("Erro ao criar o socket.");
    	    continue;
    	}

		//logSuccess("Criou o socket, tentativa de conectar.");
	    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
	        sprintf(buffer,"Erro ao tentar conectar com o servidor: %s", request->hostname);
			logSuccess(buffer);
			bzero(buffer, 4000);
	        close(sockfd);
	        continue;
	    }

		//logSuccess("Conectou no socket do servidor.");
    	break;
	}

	if( p == NULL){
		sprintf(buffer,"Nao foi possivel conectar ao servidor: %s", request->hostname);
		logSuccess(buffer);
		bzero(buffer, 4000);
		pthread_exit(NULL);
	} else{
		sprintf(buffer,"Conexao estabelecida com o host: %s", request->hostname);
		logSuccess(buffer);
		bzero(buffer, 4000);
	}

	context.socket = sockfd;
	context.sockAddr = NULL;

	sendRequest(&context, request);

	response = httpReceiveResponse(&context);

	if(response == NULL){
		sprintf(buffer,"Erro ao realizar o parse da response do host: %s", request->hostname);
		logSuccess(buffer);
		bzero(buffer, 4000);
	} else{
		sprintf(buffer,"Response recebida e parse realizado do host: %s", request->hostname);
		logSuccess(buffer);
		bzero(buffer, 4000);
	}


	close(sockfd);
	freeaddrinfo(servinfo);
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

		for(i = 0; i < request->headerCount; ++i){
			if(request->headers[i].name != NULL){
				free(request->headers[i].name);
			}
			if(request->headers[i].value != NULL){
				free(request->headers[i].value);
			}
		}

		free(request);
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

		for(i = 0; i < response->headerCount; ++i){
			//printf("\n\n\n\n\n\n\n\nNAME: %s - Size: %d\n\n\n\n\n\n\n\n\n\n", response->headers[i].name, strlen(response->headers[i].name));
			if(response->headers[i].name != NULL){
				free(response->headers[i].name);
			}
			if(response->headers[i].value != NULL){
				free(response->headers[i].value);
			}
		}

		free(response);
	}


	return 1;
}

/**

	"httpReceiveResponse": A partir do socket presente no context, a função recebe uma response Http completa e realiza seu devido parse. A response é mapeada para uma struct do tipo HttpResponse, alocada dinâmicamente.

*/
HttpResponse *httpReceiveResponse(ThreadContext *context){
	int length, size, has_body = 0, is_chunked = 0, i;

	/* Buffer de 3000, pois foi pesquisado que URL's com mais de 2000 caracteres podem não funcionar em todos as conexões cliente-servidor.*/
	char buffer[BUFFER_SIZE], buff;
	HttpResponse *response = (HttpResponse *)malloc(sizeof(HttpResponse));
	if(response == NULL){
		logError("Malloc deu erro. Linha 260");
		freeResources(context);
		pthread_exit(NULL);
	}

	/* Todos os ponteiros são inicializados com NULL e o status code inicial é de -1 para mostrar que ele não foi recebido ainda. */
	response->version = NULL;
	response->statusCode = -1;
	response->reasonPhrase = NULL;
	response->body = NULL;
	response->headers = NULL;
	response->headerCount = 0;
	response->bodySize = 0;

	size = 0;

	/* A função bzero(void *ptr, int size) escreve 0 em size posições do vetor ptr. */
	bzero(buffer, BUFFER_SIZE);


	/*

		Loop que recebe a primeira linha da resposta.
		A leitura é feita caractere a caractere do Socket.

		São lidos caracteres até encontrar o primeiro espaço. O que foi lido até aí é a versão, que é copiada para memória alocada para a struct de resposta.

		Novamente são lidos caracteres até encontrar um novo espaço. O que foi lido até este ponto foi o código de status, string que é transformada em um short para ser armazenado npo struct.

		O próximo campo a ser preenchido é o de reason phrase. Como este campo pode conter espaçoes, primeiro é verificado se o status code e a versão já foram adquirdos, para que sejam lidos caracteres até que seja encontrado um caractere '\r'. Só aí é alocado espaço para o reason phrase e então a struct recebe o ponteiro com os dados alocados.

	*/
	while(1){
		/* Leitura byte a byte do Socket. */
		length = recv(context->socket, &buff, 1, MSG_WAITALL);
		if(length < 1){
			printf("Error on receiving data from socket on httpReceiveResponse. Size < 0.\n");
			pthread_exit(NULL);
		}

		/*

			Trigger de saída do loop. Quando o primeiro '\n' é encontrado, a pirmeira linha da requisição foi totalmente adquirida.

		*/
		if(buff == '\n'){
			bzero(buffer, BUFFER_SIZE);
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
				if(response->version == NULL){
					logError("Malloc deu erro. Linha 338.");
					freeResources(context);
					pthread_exit(NULL);
				}
				for (i = 0; i < size; i++) {
					response->version[i] = buffer[i];
				}
				response->version[size] = '\0';
				//printf("-------------------------> RESPONSE - Version: %s\n", response->version);
				bzero(buffer, BUFFER_SIZE);
				size = 0;
			} else if(response->statusCode == -1){
				response->statusCode = (short)atoi(buffer);
				//printf("-------------------------> RESPONSE - Code: %d\n", response->statusCode);
				bzero(buffer, BUFFER_SIZE);
				size = 0;
			} else if(buff == '\r'){
				response->reasonPhrase = (char *)malloc((size+1)*sizeof(char));
				if(response->reasonPhrase == NULL){
					logError("Malloc deu erro. Linha 354.");
					freeResources(context);
					pthread_exit(NULL);
				}
				//strcpy(response->reasonPhrase, buffer);
				for (i = 0; i < size; i++) {
					response->reasonPhrase[i] = buffer[i];
				}
				response->reasonPhrase[size] = '\0';
				//printf("-------------------------> RESPONSE - Phrase: %s\n", response->reasonPhrase);
				bzero(buffer, BUFFER_SIZE);
				size = 0;
			}

		} else{
			buffer[size] = buff;
			++size;
		}
	}

	/* Após a primeira linha da resposta ser adquirida, os headers são pegos. Na resposta o último parâmetro é NULL pelo fato de não ser armazenado na struct o Hostname. */
	response->headers = getHeaders(context, &(response->headerCount), &has_body, &(response->bodySize), NULL, &is_chunked);

	/* Caso a resposta tenha corpo, ele é adquirido com a função getBody. */
	if( has_body == 1 ){
		response->body = getBody(context, &(response->bodySize), is_chunked);
	}

	//ResponsePrettyPrinter(response);
	return response;
}


/**

	"httpReceiveRequest": A partir do socket presente no context, a função recebe uma request Http completa e realiza seu devido parse. A request é mapeada para uma struct do tipo HttpRequest, alocada dinâmicamente.

*/

HttpRequest *httpReceiveRequest(ThreadContext *context){
	int length, size, has_body = 0, is_chunked = 0, i;
	/* Buffer de 100000, pois foi pesquisado que URL's com mais de 2000 caracteres podem não funcionar em todos as conexões cliente-servidor.*/
	char buffer[BUFFER_SIZE], buff;
	HttpRequest *request = (HttpRequest *)malloc(sizeof(HttpRequest));
	if(request == NULL){
		logError("Malloc deu erro. Linha 407.");
		freeResources(context);
		pthread_exit(NULL);
	}

	/*

		Todos os ponteiros são inicializados com NULL.

	*/
	request->method = NULL;
	request->uri = NULL;
	request->version = NULL;
	request->hostname = NULL;
	request->body = NULL;
	request->headerCount = 0;
	request->headers = NULL;
	request->bodySize = 0;

	size = 0;
	/* A função bzero(void *ptr, int size) escreve 0 em size posições do vetor ptr. */
	bzero(buffer, BUFFER_SIZE);

	/*

		Loop que recebe a primeira linha da requisição.
		A leitura é feita caractere a caractere do Socket.

		São lidos caracteres até encontrar o primeiro espaço. O que foi lido até aí foi o método da requisição, que é copiada para memória alocada para a struct de request.

		Novamente são lidos caracteres até encontrar um novo espaço. O que foi lido até este ponto foi a URI, então espaço é alocado para ela na struct de resposta.

		Então, finalmente é lida a versão Http da requisição, que também é armazenada na struct na forma de ponteiro.

	*/

	while(1){
		/* Leitura byte a byte do Socket. */
		length = recv(context->socket, &buff, 1, MSG_WAITALL);
		if(length < 1){
			printf("Error on receiving data from socket on httpReceiveRequest. Size < 0. Errno: %d.\n", errno);
			freeResources(context);
			pthread_exit(NULL);
		}

		/*

			Trigger de saída do loop. Quando o primeiro '\n' é encontrado, a pirmeira linha da requisição foi totalmente adquirida. O buffer é zerado e há a saída do loop.

		*/
		if(buff == '\n'){
			bzero(buffer, BUFFER_SIZE);
			break;
		}


		/*

			Caso o caractere lido for ' ' ou '\r', significa que é o momento de armazenar algum dos campos da resposta. Primeiro é adquirido o método da requisição, em seguida a URI da requisição e fianlmente a versão do Http.

			A memória para armazenar cada uma dessas informações é alocada dinâmicamente e as strings são copiadas do buffer.

		*/
		if(buff == ' ' || buff == '\r'){
			if(request->method == NULL){
				request->method = (char *)malloc((size+1)*sizeof(char));
				if(request->method == NULL){
					logError("Malloc deu erro. Linha 487.");
					freeResources(context);
					pthread_exit(NULL);
				}
				for (i = 0; i < size; i++) {
					request->method[i] = buffer[i];
				}
				request->method[size] = '\0';
				//printf("---------------------> REQUEST: Method: %s\n", request->method);
			} else{
				if(request->uri == NULL){
					request->uri = (char *)malloc((size+1)*sizeof(char));
					if(request->uri == NULL){
						logError("Malloc deu erro. Linha 497.");
						freeResources(context);
						pthread_exit(NULL);
					}
					for (i = 0; i < size; i++) {
						request->uri[i] = buffer[i];
					}
					request->uri[size] = '\0';
					//printf("---------------------> REQUEST: Uri: %s\n", request->uri);
				} else{
					request->version = (char *)malloc((size+1)*sizeof(char));
					if(request->version == NULL){
						logError("Malloc deu erro. Linha 506.");
						freeResources(context);
						pthread_exit(NULL);
					}
					for (i = 0; i < size; i++) {
						request->version[i] = buffer[i];
					}
					request->version[size] = '\0';
					//printf("---------------------> REQUEST: Version: %s\n", request->version);
				}
			}

			bzero(buffer, BUFFER_SIZE);
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
	request->headers = getHeaders(context, &(request->headerCount), &has_body, &request->bodySize, &request->hostname, &is_chunked);

	if(context->inspect){
		has_body = 0;
		request->bodySize = 0;
		graphicInterface(request);
		RequestPrettyPrinter(request);
	}

	/* Caso a requisição tenha corpo, ele é adquirido com a função getBody. */
	if( has_body == 1 ){
		request->body = getBody(context, &request->bodySize, is_chunked);
	}

	//RequestPrettyPrinter(request);

	return request;
}

/*

	Função que a partir de um socket do contexto, lê todos os headers de uma requisição ou resposta.

	O ponteiro para o raw da requisição, o contador de headers, o tamanho da requisição, a flag has_body (que é ativada se a requisição/resposta tem corpo), a variável body_size (que armazena o tamanho do corpo da requisição/resposta) e o ponteiro de hostname	são passados por referência porque seus valores podem ser modificados dentro da função.

	O loop principal da função só termina quando são encontrados em sequência os caracters "\r\n\r\n", sequência que simboliza o final dos headers.

	A estrutura que armazena um header é composta por um "name", que simboliza o campo do header, e "value", que é valor do campo, que inclusive pode conter espaços.

*/
HeaderField *getHeaders(ThreadContext *context, int *headerCount, int *has_body, int *body_size, char **hostname, int *is_chunked){
	int length = 0, found_linebreak = 0, size = 0, found_name = 0, i;
	char buff, buffer[BUFFER_SIZE], *value = NULL, *name = NULL;
	HeaderField headers[1000], *retHeaders;

	(*headerCount) = 0;

	bzero(buffer,BUFFER_SIZE);
	while(1){
		length = recv(context->socket, &buff, 1, MSG_WAITALL);
		if(length < 1){
			printf("Error on receiving data from socket on getHeaders. Size < 0.\n");
			logSuccess("Error on receiving data from socket on getHeaders. Size < 0.\n");
			freeResources(context);
			pthread_exit(NULL);
		}

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
			// headers = (HeaderField *)realloc(headers, ((*headerCount)+1)*sizeof(HeaderField));
			// if(headers == NULL){
			// 		logError("Realloc deu erro. Linha 619. Headers");
			// 		freeResources(context);
			// 		pthread_exit(NULL);
			// 	}
			//printf("Deu push.\n");
			//printf("Antes - Name: %s\nValue:%s\n\n", name, value);
			headers[*headerCount].name = name;
			headers[*headerCount].value = value;
			//printf("Depois - Name: %s\nValue:%s\n\n", headers[*headerCount].name, headers[*headerCount].value);
			++(*headerCount);

			//printf("Value: %s\n", value);
			if(strcmp("Transfer-Encoding", name) == 0 && (strstr(value, "chunked") != NULL || strstr(value, "chunked,") != NULL)){
				(*is_chunked) = 1;
				(*has_body) = 1;
			}

			found_linebreak = 1;
			size = 0;
			bzero(buffer, BUFFER_SIZE);
			continue;
		}


		/*

			Se o caractere encontrado é um espaço e o "name" do header não foi definido, é alocada memória para ele e então seu valor é copiado do buffer que o armazenado, sendo este buffer zerado em seguida. O caractere ':' não é armazenado no "name".

			Se o "name" do header foi encontrado e o caractere atual é um '\r', todo o conteúdo de "value" já está atualmente no buffer, então espaço para o "value" é alocado dinâmicamente e seu valor é copiado do buffer, que é zerado em seguida.

			Caso as condições acima não sejam satisfeitas, o caractere atual é armazenado no buffer.

		*/
		if(buff == ' ' && found_name == 0){
			name = (char *)calloc(size, sizeof(char));
			if(name == NULL){
					logError("Malloc deu erro. Linha 654. Name");
					freeResources(context);
					pthread_exit(NULL);
			}
			for ( i = 0; i < (size-1); i++) {
					name[i] = buffer[i];
			}
			name[size-1] = '\0';
			found_name = 1;


			bzero(buffer, BUFFER_SIZE);
			size = 0;
		} else if(buff == '\r' && found_name == 1){
			if((size+1) > BUFFER_SIZE){
				printf("\n\n\n\n\n\n\n\n\n\nSize: %d\nBuffer: %s\n\n\n\n\n\n\n\n", size+1, buffer);
				exit(1);
			}
			value = (char *)calloc((size+1), sizeof(char));
			if(value == NULL){
				logError("Malloc deu erro. Linha 667. Value");
				freeResources(context);
				pthread_exit(NULL);
			}
			for ( i = 0; i < size; i++) {
					value[i] = buffer[i];
			}

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
			bzero(buffer, BUFFER_SIZE);
			size = 0;
		} else {
			buffer[size] = buff;
			++size;
		}
	}

	retHeaders = (HeaderField *)calloc((*headerCount), sizeof(HeaderField));
	for (size = 0; size < (*headerCount); size++) {
		retHeaders[size].name = headers[size].name;
		retHeaders[size].value = headers[size].value;
	}
	/* Após adquirir todos os headers, a função retorna um ponteiro de HeaderField alocado dinâmicamente. */
	return retHeaders;
}

/*
	Função que lê uma linha da requisição que possui o tamanho do chunk de body a ser lido, retornando como inteiro esse tamanho.
*/
int getChunkedSize(ThreadContext *context, char **body, int *bodySize) {
	char buffer[100];
	char byte;
	int bufSize = 0;
	int length;
	int parsed, i;

	bzero(buffer, 100);

	while (TRUE) {
		length = recv(context->socket, &byte, 1, MSG_WAITALL);
		if(length < 1){
			printf("Error on receiving data from socket on getChunkedSize. Size < 0.\n");
			logError("Error on receiving data from socket on getChunkedSize. Size < 0.\n");
			freeResources(context);
			pthread_exit(NULL);
		}
		//printf("Char: '%c'\n", byte);
		buffer[bufSize] = byte;
		++bufSize;
		if (byte == '\n') {
			break;
		}
	}

	(*body) = (char *)realloc((*body), ((*bodySize) + bufSize)*sizeof(char));
	if((*body) == NULL){
		logError("Realloc deu erro. Linha 738. Body");
		freeResources(context);
		pthread_exit(NULL);
	}
	for (i = 0; i < bufSize; i++) {
		(*body)[(*bodySize)+i] = buffer[i];
	}
	(*bodySize) = (*bodySize) + bufSize;

	sscanf(buffer, "%x\r\n", &parsed);

	return parsed;
}

/*

	A partir de um socket armazenado num contexto, são lidos "body_size" bytes do Socket, bytes esses que compõe o corpo da requisção. O corpo também é armazenado no Raw da requisição.

	As variáveis raw e req_size são passadas por referência pois tem seu valor modificado dentro da função.

*/
char *getBody(ThreadContext *context, int *body_size, int is_chunked){
	int length, size = 0, i;
	char *body = NULL, *buffer, buff[2], errorBuffer[300];

	if(is_chunked == 0){
		/* É alocada memória para o corpo da requisição. */
		body = (char *)malloc((*body_size)*sizeof(char));
		if(body == NULL){
			logError("Malloc deu erro. Linha 768. Body");
			freeResources(context);
			pthread_exit(NULL);
		}

		/* A leitura é feita para o espaço criado para o corpo. */
		length = recv(context->socket, body, (*body_size), MSG_WAITALL);
		if(length < (*body_size)){
			printf("Error on receiving data from socket on getBody. Size < 0.\n");
			logSuccess("Error on receiving data from socket on getBody. No chunked. Size < 0.\n");
			freeResources(context);
			pthread_exit(NULL);
		}

		/*

			O espaço de aramazenamento do raw da requisição é aumentado para caber os bytes do corpo 	da requisição. A cópia é realizada byte a byte.

		*/

		/* O ponteiro é retornado com os dados do corpo. */
		return body;
	} else{

		//printf("----------------------> Is chunked!\n");
		while(1){
			size = getChunkedSize(context, &body, body_size);
			//printf("SIZE: %d\n", size);

			if(size < 1){
				length = recv(context->socket, buff, 2*sizeof(char), MSG_WAITALL);
				if(length < 2){
					bzero(errorBuffer, 300);
					sprintf(errorBuffer, "Error on receiving 2 chars from socket. Errno: %d.\n", errno);
					logSuccess(errorBuffer);
					printf("Errno: %d\n", errno);
					freeResources(context);
					pthread_exit(NULL);
				}

				body = (char *)realloc(body, ((*body_size)+2)*sizeof(char));
				if(body == NULL){
					logError("Realloc deu erro. Linha 829. Body");
					freeResources(context);
					pthread_exit(NULL);
				}
				body[(*body_size)] = '\r';
				body[(*body_size+1)] = '\n';
				*body_size = (*body_size) + 2;

				break;

			} else{
				buffer = (char *)malloc((size+2)*sizeof(char));
				if(buffer == NULL){
					logError("Realloc deu erro. Linha 844. Buffer");
					freeResources(context);
					pthread_exit(NULL);
				}
				length = recv(context->socket, buffer, (size+2)*sizeof(char), MSG_WAITALL);
				if(length < (size+2)){
					logSuccess("Error on receiving data from socket. Chunked. GetBody com size maior que 0.\n");
					freeResources(context);
					pthread_exit(NULL);
				}
				//printf("Size: %d - Length: %d\n", size, length);

				body = (char *)realloc(body, ((*body_size)+size+2)*sizeof(char));
				if(body == NULL){
					logError("Realloc deu erro. Linha 856. Body");
					freeResources(context);
					pthread_exit(NULL);
				}


				for (i = 0; i < (size+2); ++i){
					body[(*body_size)+i] = buffer[i];
				}
				*body_size = (*body_size) + size + 2;
				free(buffer);
			}
		}
	}

	return body;
}


/**

	"httpParseResponse": A partir do string Raw de uma response Http completa e realiza seu devido parse. A response é mapeada para uma struct do tipo HttpResponse, alocada dinâmicamente.

*/
HttpResponse *httpParseResponse(char *resp, int length) {
	int size, body_size = 0, req_size, has_body = 0, i;

	/* Buffer de 3000, pois foi pesquisado que URL's com mais de 2000 caracteres podem não funcionar em todos as conexões cliente-servidor.*/
	char buffer[BUFFER_SIZE], buff;
	HttpResponse *response = (HttpResponse *)malloc(sizeof(HttpResponse));
	if(response == NULL){
		logError("Realloc deu erro. Linha 896. Response");
		pthread_exit(NULL);
	}

	/* Todos os ponteiros são inicializados com NULL e o status code inicial é de -1 para mostrar que ele não foi recebido ainda. */
	response->version = NULL;
	response->statusCode = -1;
	response->reasonPhrase = NULL;
	response->body = NULL;
	response->headers = NULL;
	response->headerCount = 0;


	req_size = 0;
	size = 0;
	bzero(buffer, BUFFER_SIZE);


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
			bzero(buffer, BUFFER_SIZE);
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
				if(response->version == NULL){
					logError("Realloc deu erro. Linha 961. Version");
					pthread_exit(NULL);
				}
				for (i = 0; i < size; i++) {
					response->version[i] = buffer[i];
				}
				response->version[size] = '\0';
				//printf("-------------------------> Response Version: %s\n", response->version);
				bzero(buffer, BUFFER_SIZE);
				size = 0;
			} else if(response->statusCode == -1){
				response->statusCode = (short)atoi(buffer);
				//printf("-------------------------> Response Code: %d\n", response->statusCode);
				bzero(buffer, BUFFER_SIZE);
				size = 0;
			} else if(buff == '\r'){
				response->reasonPhrase = (char *)malloc((size+1)*sizeof(char));
				if(response->reasonPhrase == NULL){
					logError("Realloc deu erro. Linha 977. Phrase");
					pthread_exit(NULL);
				}
				for (i = 0; i < size; i++) {
					response->reasonPhrase[i] = buffer[i];
				}
				response->reasonPhrase[size] = '\0';
				//printf("-------------------------> Response Phrase: %s\n", response->reasonPhrase);
				bzero(buffer, BUFFER_SIZE);
				size = 0;
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
		response->bodySize = (length - req_size);
		response->body = getLocalBody(resp, &req_size, (length - req_size));
	}


	//ResponsePrettyPrinter(response);
	return response;
}


/*

	Função que a partir de uma string com uma requisição raw, lê todos os headers de uma requisição ou resposta.

	O contador de headers, o tamanho da requisição, a flag has_body (que é ativada se a requisição/resposta tem corpo), a variável body_size (que armazena o tamanho do corpo da requisição/resposta) e o ponteiro de hostname	são passados por referência porque seus valores podem ser modificados dentro da função.

	O loop principal da função só termina quando são encontrados em sequência os caracters "\r\n\r\n", sequência que simboliza o final dos headers.

	A estrutura que armazena um header é composta por um "name", que simboliza o campo do header, e "value", que é valor do campo, que inclusive pode conter espaços.

*/
HeaderField *getLocalHeaders(char *resp, int *headerCount, int *req_size, int *has_body, int *body_size, char **hostname){
	int found_linebreak = 0, size = 0, found_name = 0, i;
	char buff = '\0', buffer[BUFFER_SIZE], *value = NULL, *name = NULL;
	HeaderField *headers = NULL;

	bzero(buffer, BUFFER_SIZE);
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
			if(headers == NULL){
					logError("Realloc deu erro. Linha 1055. Headers.");
					pthread_exit(NULL);
				}
			headers[*headerCount].name = name;
			headers[*headerCount].value = value;
			++(*headerCount);

			if(strcmp("Transfer-Encoding", name) == 0 && (strstr(value, "chunked") != NULL || strstr(value, "chunked,") != NULL)){
				(*has_body) = 1;
			}

			found_linebreak = 1;
			size = 0;
			bzero(buffer, BUFFER_SIZE);
			continue;
		}


		/*

			Se o caractere encontrado é um espaço e o "name" do header não foi definido, é alocada memória para ele e então seu valor é copiado do buffer que o armazenado, sendo este buffer zerado em seguida. O caractere ':' não é armazenado no "name".

			Se o "name" do header foi encontrado e o caractere atual é um '\r', todo o conteúdo de "value" já está atualmente no buffer, então espaço para o "value" é alocado dinâmicamente e seu valor é copiado do buffer, que é zerado em seguida.

			Caso as condições acima não sejam satisfeitas, o caractere atual é armazenado no buffer.

		*/
		if(buff == ' ' && found_name == 0){
			name = (char *)malloc(size*sizeof(char));
			if(name == NULL){
					logError("Realloc deu erro. Linha 1086. Name");
					pthread_exit(NULL);
				}
				for (i = 0; i < (size-1); i++) {
					name[i] = buffer[i];
				}
			name[size-1] = '\0';
			found_name = 1;


			bzero(buffer, BUFFER_SIZE);
			size = 0;
		} else if(buff == '\r' && found_name == 1){
			value = (char *)malloc((size+1)*sizeof(char));
			if(value == NULL){
					logError("Realloc deu erro. Linha 1099. Value");
					pthread_exit(NULL);
				}
			for (i = 0; i < size; i++) {
				value[i] = buffer[i];
			}
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
			bzero(buffer, BUFFER_SIZE);
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
	char *body;
	int readings;

	body = (char *)malloc((body_size)*sizeof(char));
	if(body == NULL){
					logError("Realloc deu erro. Linha 1147. Body");
					pthread_exit(NULL);
				}

	readings = stringCopy(body, &(resp[(*req_size)]), body_size);
	if(readings < body_size){
		printf("Leitura não para body não realizado.\n");
		pthread_exit(NULL);
	}

	(*req_size) += body_size;

	//printf("Local body: %s\n\n", body);

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

	printf("Body size: %d\n", response->bodySize);
	// if(response->body != NULL){
	// 	for (i = 0; i < response->bodySize; ++i){
	// 		printf("%c", response->body[i]);
	// 	}
	// 	printf("\n");
	// }
	printf("\n");
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

	printf("Body size: %d\n", request->bodySize);
	// if(request->body != NULL){
	// 	for (i = 0; i < request->bodySize; ++i){
	// 		printf("%c", request->body[i]);
	// 	}
	// 	printf("\n");
	// }

}

/*
	Função que cria uma response específica para ocorrência de um hostname em blackList.
*/
HttpResponse *blacklistResponseBuilder(){
	HttpResponse *response;
	time_t now = time(0);
	struct tm tm = *gmtime(&now);
	char buffer[100];

	bzero(buffer, 100);

	response = (HttpResponse *)malloc(sizeof(HttpResponse));

	response->version = NULL;
	response->statusCode = 403;
	response->reasonPhrase = NULL;
	response->body = NULL;
	response->headers = NULL;
	response->headerCount = 2;
	response->bodySize = 0;

	response->version = (char *)calloc((strlen("HTTP/1.1")+1), sizeof(char));
	strcpy(response->version, "HTTP/1.1");

	response->reasonPhrase = (char *)calloc((strlen("Request denied: On Blacklist.")+1), sizeof(char));
	strcpy(response->reasonPhrase, "Request denied: On Blacklist.");

	response->headers = (HeaderField *)calloc(response->headerCount, sizeof(HeaderField));

	response->headers[0].name = (char *)calloc((strlen("Connection")+1), sizeof(char));
	strcpy(response->headers[0].name, "Connection");

	response->headers[0].value = (char *)calloc((strlen("close")+1), sizeof(char));
	strcpy(response->headers[0].value, "close");

	response->headers[1].name = (char *)calloc((strlen("Date")+1), sizeof(char));
	strcpy(response->headers[1].name, "Date");

	strftime(buffer, sizeof buffer, "%a, %d %b %Y %H:%M:%S %Z", &tm);
	response->headers[1].value = (char *)calloc((strlen(buffer)+1), sizeof(char));
	strcpy(response->headers[1].value, buffer);


	return response;
}

/*
	Função que cria uma response específica para ocorrência de um denied term em uma requisição ou responsta. A flag "is_response" indica o tipo, modificando a reason phrase.
*/
HttpResponse *deniedTermsResponseBuilder(ValidationResult *validation, int is_response){
	HttpResponse *response;
	time_t now = time(0);
	struct tm tm = *gmtime(&now);
	char buffer[1000];

	bzero(buffer, 1000);

	response = (HttpResponse *)malloc(sizeof(HttpResponse));

	response->version = NULL;
	response->statusCode = 403;
	response->reasonPhrase = NULL;
	response->body = NULL;
	response->headers = NULL;
	response->headerCount = 2;
	response->bodySize = 0;

	response->version = (char *)calloc((strlen("HTTP/1.1")+1), sizeof(char));
	strcpy(response->version, "HTTP/1.1");
	if(is_response){
		sprintf(buffer, "Response denied on term '%s'", validation->deniedTerm);
		response->reasonPhrase = (char *)calloc(strlen(buffer)+1, sizeof(char));
	} else{
		sprintf(buffer, "Request denied on term '%s'", validation->deniedTerm);
		response->reasonPhrase = (char *)calloc(strlen(buffer)+1, sizeof(char));
	}
	strcpy(response->reasonPhrase, buffer);
	bzero(buffer, 100);


	response->headers = (HeaderField *)calloc(response->headerCount, sizeof(HeaderField));

	response->headers[0].name = (char *)calloc((strlen("Connection")+1), sizeof(char));
	strcpy(response->headers[0].name, "Connection");

	response->headers[0].value = (char *)calloc((strlen("close")+1), sizeof(char));
	strcpy(response->headers[0].value, "close");

	response->headers[1].name = (char *)calloc((strlen("Date")+1), sizeof(char));
	strcpy(response->headers[1].name, "Date");

	strftime(buffer, sizeof buffer, "%a, %d %b %Y %H:%M:%S %Z", &tm);
	response->headers[1].value = (char *)calloc((strlen(buffer)+1), sizeof(char));
	strcpy(response->headers[1].value, buffer);


	return response;
}

/*
	Função que a partir de uma lista de headers, gera uma string concatenando todos os headers. A string gerada é igual ao conjunto de headers em formato Raw que está presente em uma requisição. Serve para ser colocada na janela de edição de headers.
*/
char *GetHeadersString(HeaderField *headers, int headerCount){
	int i, size = 0, valueSize = 0, nameSize = 0;
	char buffer[1000000], *str;

	bzero(buffer, 1000000);

	for(i = 0; i < headerCount; ++i){
		nameSize = strlen(headers[i].name);
		valueSize = strlen(headers[i].value);
		sprintf(&(buffer[size]), "%s: %s\r\n", headers[i].name, headers[i].value);
		size += valueSize + nameSize + 4;
	}

	str = (char *)calloc(size+1, sizeof(char));
	strcpy(str, buffer);

	return str;
}
