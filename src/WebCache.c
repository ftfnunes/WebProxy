#include "WebCache.h"

// retorna um HttpResponse alocado dinamicamente.
HttpResponse *getResponseFromCache(HttpRequest* request) {
	char *hash = calculateHash(request->raw);
	char filename[CACHE_FILENAME_SIZE];
	char *file = NULL;
	HttpResponse *httpResponse = NULL;

	sprintf(filename, "%s.cache", hash);

	if (access(filename, F_OK) == 0) {
		//O arquivo existe
		file = readAsString(filename);
		httpResponse = httpParseResponse(file);
		return httpResponse;
	}

	return NULL;
}

int isExpired(HttpResponse *response){
	char *expiresDateStr = NULL;
	char *responseDateStr = NULL;
	time_t expiresDate;
	time_t now;

	time(&now);
	expiresDateStr = findHeaderByName(EXPIRES_HEADER, response->headers, response->headerCount);

	if (expiresDateStr != NULL) {
		expiresDate = convertToTime(expiresDateStr, 0);
		if (expiresDate > now) {
			return TRUE;
		}
	} else {
		responseDateStr = findHeaderByName(DATE_HEADER, response->headers, response->headerCount);
		if (responseDateStr == NULL) {
			return TRUE;;
		}
		expiresDate = convertToTime(responseDateStr, CACHED_RESPONSE_LIFETIME);
		if (expiresDate > now) {
			return TRUE;;
		}
	}

	return FALSE;
}

void storeInCache(HttpResponse *response, HttpRequest* request) {
	char *hash = calculateHash(request->raw);
	char filename[CACHE_FILENAME_SIZE];
	FILE *fp;

	sprintf(filename, "%s.cache", hash);

	fp = fopen(filename, "w");

	if (fp == NULL) {
		//TODO: Tratamento de erro
	}

	fwrite(response->raw, strlen(request->raw), 1, fp);

	fclose(fp);
}

//funções utilitárias

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

	if (fp == NULL) {
		//TODO: Tratamento de erro
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

