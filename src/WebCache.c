#include "WebCache.h"

// Recebe um request e verifica se há alguma response correspondente já em cache
// Retorna essa response caso encontre, ou NULL caso contrário.
// Caso o arquivo seja encontrado, este arquivo vai para o final da fila de arquivos
// acessados menos recentemente.

HttpResponse *getResponseFromCache(HttpRequest* request) {
	char *filename = getRequestFilename(request);
	char key[HASH_SIZE];
	char *file = NULL;
	char *logStr = (char *)malloc((BUFFER_SIZE)*sizeof(char));
	HttpResponse *httpResponse = NULL;
	int length;

	getKeyFromFilename(key, filename);

	if (access(filename, F_OK) == 0) {
		//O arquivo existe
		file = readAsString(filename, &length);

		pthread_mutex_lock(&queueMutex);
		push(fileQueue, key);
		pthread_mutex_unlock(&queueMutex);

		httpResponse = httpParseResponse(file, length);

		sprintf(logStr, "Successfully obtained response to request:\n %.200s", file);
		logSuccess(logStr);
	}

	free(logStr);
	free(filename);
	return httpResponse;
}


// Recebe uma response, caso essa os cabeçalhos dessa response não estejam de acordo
// com a política de expiração do proxy, retorna TRUE, caso contrário retorna FALSE.
// A resposta é considerada expirada se a hora atual for maior do que a data indicada no cabeçalho "Expires"
// ou se a hora atual for maior do que a data de emissão mais CACHED_RESPONSE_LIFETIME, caso o cabeçalho expires
// não esteja presente
int isExpired(HttpResponse *response){
	char *expiresDateStr = NULL;
	char *responseDateStr = NULL;
	time_t expiresDate;
	time_t now = time(NULL);
	int isExpired = FALSE;

	expiresDateStr = findHeaderByName(EXPIRES_HEADER, response->headers, response->headerCount);

	if (expiresDateStr != NULL) {
		expiresDate = convertToTime(expiresDateStr, 0);
	} else {
		responseDateStr = findHeaderByName(DATE_HEADER, response->headers, response->headerCount);
		if (responseDateStr == NULL) {
			isExpired = TRUE;
		}
		expiresDate = convertToTime(responseDateStr, CACHED_RESPONSE_LIFETIME);
	}
	if (expiresDate < now) {
		isExpired = TRUE;
	}

	return isExpired;
}

// Indica se a resposta pode ser inserida no cache. A resposta pode ser cacheada se
// não contiver no cabeçalho Cache-Control as diretivas private e no-cache
int shouldBeCached(HttpResponse *response) {
	char *cacheControl = findHeaderByName(CACHE_CONTROL_HEADER, response->headers, response->headerCount);
	char *buffer;
	char *token;
	char *saveptr;

	if (cacheControl != NULL) {
		buffer = (char *)malloc((strlen(cacheControl)+1)*sizeof(char));
		strcpy(buffer, cacheControl);

		for (token = strtok_r(buffer, " ,", &saveptr); token != NULL; token = strtok_r(NULL, " ", &saveptr)) {
			if (strcmp(token, NO_CACHE_DIRECTIVE) == 0 || strcmp(token, PRIVATE_DIRECTIVE) == 0) {
				free(buffer);
				return FALSE;
			}
		}
	}
	free(buffer);
	return TRUE;
}

// Armazena uma nove response na cache, de acordo com a request recebida.
// Os arquivos de cache são considerados um recurso compartilhado entre as
// threads, logo, utilizam de um lock para sincronização.
// O arquivo novo é colocado no final da fila de arquivos acessados menos recentemente
void storeInCache(HttpResponse *response, HttpRequest* request) {
	char *filename = getRequestFilename(request);
	char logStr[200];
	char key[HASH_SIZE];
	FILE *fp;
	int responseLength;
	char *responseRaw = getResponseRaw(response, &responseLength);
	struct stat fileStat;

	getKeyFromFilename(key, filename);

	pthread_mutex_lock(&cacheMutex);

	if (access(filename, F_OK) == 0) {
		if (stat(filename, &fileStat) != 0) {
			sprintf(logStr, "Could not get stats for file %s.", filename);
			logError(logStr);
			free(responseRaw);
			return;
		}
		cacheSize -= fileStat.st_size;
	}

	while (cacheSize + responseLength > maxSize) {
		if (!removeLRAResponse()) {
			logWarning("No space available in cache.");
			free(responseRaw);
			return;
		}
	}

	fp = fopen(filename, "w");

	if (fp == NULL) {
		sprintf(logStr, "The file %s could not be opened.", filename);
		logError(logStr);
		pthread_mutex_unlock(&cacheMutex);
		return;
	}

	fwrite(responseRaw, sizeof(char), responseLength, fp);

	fclose(fp);
	pthread_mutex_lock(&queueMutex);
	push(fileQueue, key);
	pthread_mutex_unlock(&queueMutex);

	cacheSize += responseLength;
	pthread_mutex_unlock(&cacheMutex);

	free(responseRaw);
	free(filename);
}

// Transforma uma response em uma string com a representação exata de uma resposta
// HTTP.
char *getResponseRaw(HttpResponse *response, int *length) {
	char *raw = NULL;
	int size = 0;
	int oldSize = 0;
	int i;
	char codeStr[5];

	sprintf(codeStr, "%d", response->statusCode);
	size += strlen(response->version) + 1;
	size += strlen(codeStr) + 1;
	size += strlen(response->reasonPhrase) + 2;
	raw = (char *)malloc((size+1)*sizeof(char));
	sprintf(raw, "%s %d %s\r\n", response->version, response->statusCode, response->reasonPhrase);

	for (i = 0; i < response->headerCount; i++) {
		oldSize = size;
		size += strlen(response->headers[i].name) + 2;
		size += strlen(response->headers[i].value) + 2;
		raw = (char *)realloc(raw, (size+1)*sizeof(char));
		sprintf(raw + oldSize, "%s: %s\r\n", response->headers[i].name, response->headers[i].value);
	}
	size += 2;
	raw = (char *)realloc(raw, size*sizeof(char));
	raw[size-1] = '\n';
	raw[size-2] = '\r';

	if (response->bodySize != 0) {
		oldSize = size;
		size += response->bodySize;
		raw = (char *)realloc(raw, size*sizeof(char));
		for (i = 0; i < response->bodySize; i++) {
			raw[oldSize+i] = response->body[i];
		}
	}

	*length = size;
	return raw;
}

// Remove o response acessado menos recentemente da cache, para liberar espaço
int removeLRAResponse() {
	char *key;
	char *filename;
	char error[200];
	struct stat fileStat;
	off_t fileSize;

	pthread_mutex_lock(&queueMutex);
	key = pop(fileQueue);
	pthread_mutex_unlock(&queueMutex);

	if (key == NULL) {
		return FALSE;
	}

	filename = getFilenameFromKey(key);

	if (stat(filename, &fileStat) != 0) {
		sprintf(error, "Could not get stats for file %s.", filename);
		logError(error);
		return FALSE;
	}
	fileSize = fileStat.st_size;

	if (remove(filename) != 0) {
		printf(error, "Could not remove file %s.", filename);
		logError(error);
		return FALSE;
	}

	cacheSize -= fileSize;

	free(key);
	free(filename);
	return TRUE;
}

// Função que inicializa as variáveis globais utilizadas pela cache
void initializeCache(int sizeOfCache) {
	maxSize = sizeOfCache;
	fileQueue = initializeQueue();
	cacheSize = getCacheStatus(fileQueue);
	pthread_mutex_init(&cacheMutex, NULL);
	pthread_mutex_init(&queueMutex, NULL);
}

//funções utilitárias

// Calcula o tamanho do diretório que armazena as respostas cacheadas e configura a fila com
// as respostas accessadas menos recentemente.
off_t getCacheStatus(Queue *queue) {
    DIR * dirp;
    struct dirent * entry;
    struct stat structstat;
    char filename[500];
	char error[500];
    off_t size = 0;
	char key[HASH_SIZE];
	FileList *list = createFileList();

    if ((dirp = opendir(CACHE_PATH)) == NULL) {
    	logError("Could not open cache directory.");
    }

    while ((entry = readdir(dirp)) != NULL) {
        if (entry->d_type == DT_REG) {
            sprintf(filename, "%s/%s", CACHE_PATH, entry->d_name);
            if (stat(filename, &structstat) == 0) {
                size += structstat.st_size;
				getKeyFromFilename(key, filename);
				insertInOrder(list, key, structstat.st_atim.tv_sec);
            } else {
				sprintf(error, "Could not get stats for file %s.", filename);
				logError(error);
			}
        }
    }
    closedir(dirp);
	fileListToQueue(list, queue);
	freeFilesList(list);
    return size;
}

void getKeyFromFilename(char *key, char *filename) {
	strncpy(key, filename+strlen(CACHE_PATH)+1, HASH_SIZE-1);
	key[HASH_SIZE-1] = '\0';
}

char convertToHexa(unsigned char c) {
	if (c > 15) {
		printf("Invalid byte to convert!\n");
		exit(1);
	}
	if (c < 10) {
		return (char) c+'0';
	}
	return (char) c+'a'-10;
}

// Retorna a string do hash de uma url em base 16, alocada dinamicamente.
char *calculateHash(char *requestUrl) {
	char *result = malloc(HASH_SIZE);
	unsigned char hash[SHA256_DIGEST_LENGTH];
	char buffer[2];
	int i;

	SHA256((unsigned char *)requestUrl, strlen(requestUrl), hash);

	for (i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
		buffer[0] = convertToHexa(hash[i] >> 4);
		buffer[1] = convertToHexa(hash[i] & 0x0F);
		result[i*2] = buffer[0];
		result[(i*2)+1] = buffer[1];
	}
	result[i*2] = '\0';

	return result;
}

//Le um arquivo como uma string, alocada dinamicamente.
char *readAsString(char *filename, int *length) {
	FILE *fp = fopen(filename, "r");
	char *file = NULL;
	long int fileSize = 0;
	char errorStr[200];

	if (fp == NULL) {
		sprintf(errorStr, "The file %s could not be opened.", filename);
		logError(errorStr);
		return NULL;
	}

	fseek(fp, 0, SEEK_END);
	fileSize = ftell(fp);
	rewind(fp);
	file = (char *)malloc(fileSize+1);
	fread(file, fileSize, 1, fp);

	fclose(fp);
	file[fileSize] = '\0';
	*length = fileSize;
	return file;
}

// Recebe a string que representa o tempo de uma requisição e retorna a sua representação
// em segundos desde 1970
time_t convertToTime(char *dateStr, int minutesToAdd){
	struct tm date;

	strptime(dateStr, "%a, %d %b %Y %H:%M:%S GMT", &date);
	date.tm_min += minutesToAdd;
	return timegm(&date);
}

// Recupera o valor de um cabeçalho pelo nome.
char *findHeaderByName(char *name, HeaderField *headers, int headerCount) {
	int i;
	for (i = 0; i < headerCount; ++i) {
		if (strcmp(headers[i].name, name) == 0) {
			return headers[i].value;
		}
	}
	return NULL;
}

char *getFilenameFromKey(char *key) {
	char *filename = (char *)malloc((CACHE_FILENAME_SIZE+strlen(CACHE_PATH))*sizeof(char));
	sprintf(filename, "%s/%s.cache",CACHE_PATH, key);
	return filename;
}

char *getUrl(HttpRequest *request) {
	char *url;
	if (strncmp(request->uri, "/", 1) == 0) {
		url = malloc((strlen(request->uri) + strlen(request->hostname) + 1)*sizeof(char));
		strcpy(url, request->hostname);
		strcat(url, request->uri);
	} else {
		url = malloc((strlen(request->uri)+ 1)*sizeof(char));
		strcpy(url, request->uri);
	}

	return url;
}

// Retorna o nome do arquivo correspondente a uma request.
char *getRequestFilename(HttpRequest *request) {
	char *url = getUrl(request);
	char *hash = calculateHash(url);
	char *filename = getFilenameFromKey(hash);
	free(hash);
	free(url);
	return filename;
}

// As funções abaixo são utilizadas para ordenar os arquivos de cache pelo tempo desde o ultimo acesso.

FileListNode *createFileListNode(char *key, time_t lastAccess) {
	FileListNode *node = (FileListNode *)malloc(sizeof(FileListNode));

	node->key = (char *)malloc((strlen(key) + 1)*sizeof(char));
	strcpy(node->key, key);
	node->lastAccess = lastAccess;
	node->next = NULL;

	return node;
}

FileList *createFileList() {
	FileList *list = (FileList *)malloc(sizeof(FileList));

	list->start = NULL;
	return list;
}


// Converte a lista na fila de arquivos acessados menos recentemente utilizada
// no controle do tamanho da cache
void fileListToQueue(FileList *list, Queue *queue) {
	FileListNode *cur;
	for (cur = list->start; cur != NULL; cur = cur->next) {
		push(queue, cur->key);
	}
}

void freeFilesList(FileList *list) {
	FileListNode *cur;
	for ( cur = list->start; cur != NULL; cur = cur->next) {
		free(cur->key);
		free(cur);
	}
}

void insertInOrder(FileList *list, char *key, time_t lastAccess) {
	FileListNode *newNode = createFileListNode(key, lastAccess);
	FileListNode *cur, *prev;
	cur = list->start;
	prev = NULL;
	while (cur != NULL && lastAccess > cur->lastAccess) {
		prev = cur;
		cur = cur->next;
	}
	if (prev == NULL) {
		list->start = newNode;
	} else {
		prev->next = newNode;
	}
	newNode->next = cur;
}
