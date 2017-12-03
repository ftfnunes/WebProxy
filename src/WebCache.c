#include "WebCache.h"

// retorna um HttpResponse alocado dinamicamente.
HttpResponse *getResponseFromCache(HttpRequest* request) {
	char *filename = getRequestFilename(request);
	char key[HASH_SIZE];
	char *file = NULL;
	char *logStr = (char *)malloc((strlen(request->raw)+SIZE_OF_MESSAGE)*sizeof(char));
	HttpResponse *httpResponse = NULL;

	getKeyFromFilename(key, filename);

	if (access(filename, F_OK) == 0) {
		//O arquivo existe
		file = readAsString(filename);

		pthread_mutex_lock(&queueMutex);
		push(fileQueue, key);
		pthread_mutex_unlock(&queueMutex);

		httpResponse = httpParseResponse(file);

		sprintf(logStr, "Successfully obtained response to request:\n %s", request->raw);
		logSuccess(logStr);
	}

	free(logStr);
	free(filename);
	return httpResponse;
}

int isExpired(HttpResponse *response){
	char *expiresDateStr = NULL;
	char *responseDateStr = NULL;
	time_t expiresDate;
	time_t now = time(NULL);

	expiresDateStr = findHeaderByName(EXPIRES_HEADER, response->headers, response->headerCount);

	if (expiresDateStr != NULL) {
		expiresDate = convertToTime(expiresDateStr, 0);
	} else {
		responseDateStr = findHeaderByName(DATE_HEADER, response->headers, response->headerCount);
		if (responseDateStr == NULL) {
			return TRUE;
		}
		expiresDate = convertToTime(responseDateStr, CACHED_RESPONSE_LIFETIME);
	}
	if (expiresDate < now) {
		return TRUE;
	}

	return FALSE;
}

int shouldBeCached(HttpResponse *response) {
	char *cacheControl = findHeaderByName(CACHE_CONTROL_HEADER, response->headers, response->headerCount);


}

void storeInCache(HttpResponse *response, HttpRequest* request) {
	char *filename = getRequestFilename(request);
	char logStr[200];
	char key[HASH_SIZE];
	FILE *fp;
	int responseLength = strlen(response->raw);
	struct stat fileStat;

	getKeyFromFilename(key, filename);

	pthread_mutex_lock(&cacheMutex);

	if (access(filename, F_OK) == 0) {
		if (stat(filename, &fileStat) != 0) {
			sprintf(logStr, "Could not get stats for file %s.", filename);
			logError(logStr);
		}
		cacheSize -= fileStat.st_size;
	}

	while (cacheSize + responseLength > MAX_CACHE_SIZE) {
		if (!removeLRAResponse()) {
			logWarning("No space available in cache.");
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

	fprintf(fp, "%s", response->raw);

	fclose(fp);
	pthread_mutex_lock(&queueMutex);
	push(fileQueue, key);
	pthread_mutex_unlock(&queueMutex);

	cacheSize += responseLength;
	pthread_mutex_unlock(&cacheMutex);

	free(filename);
}

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


void initializeCache() {
	fileQueue = initializeQueue();
	cacheSize = getCacheStatus(fileQueue);
	pthread_mutex_init(&cacheMutex, NULL);
	pthread_mutex_init(&queueMutex, NULL);
}

//funções utilitárias

// Calcula o tamanho do diretório que armazena as respostas cacheadas e gera configura a pilha com
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

// Retorna a string do hash em base 16, alocada dinamicamente.
char *calculateHash(char *request) {
	char *result = malloc(HASH_SIZE);
	unsigned char hash[SHA256_DIGEST_LENGTH];
	char buffer[2];
	int i;

	SHA256((unsigned char *)request, strlen(request), hash);

	for (i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
		buffer[0] = convertToHexa(hash[i] >> 4);
		buffer[1] = convertToHexa(hash[i] & 0x0F);
		result[i*2] = buffer[0];
		result[(i*2)+1] = buffer[1];
	}
	result[i*2] = '\0';

	return result;
}

//Resultado é alocado dinamicamente.
char *readAsString(char *filename) {
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

	return file;
}

time_t convertToTime(char *dateStr, int minutesToAdd){
	struct tm date;

	strptime(dateStr, "%a, %d %b %Y %H:%M:%S GMT", &date);
	date.tm_min += minutesToAdd;
	return timegm(&date);
}

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

// Funções de manipulação de listas de arquivos
char *getRequestFilename(HttpRequest *request) {
	char *hash = calculateHash(request->raw);
	char *filename = getFilenameFromKey(hash);
	free(hash);
	return filename;
}

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
