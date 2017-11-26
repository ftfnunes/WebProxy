#include "WebCache.h"

HttpResponse *getResponseFromCache(HttpRequest* request) {
	char *hash = calculateHash(request->raw);
	char *filename = malloc(CACHE_FILENAME_SIZE);
	char *file = NULL;
	HttpResponse *httpResponse = NULL;

	sprintf(filename, "%s.cache", hash);

	if (access(filename, F_OK) == 0) {
		//O arquivo existe
		file = readAsString(filename);
		httpResponse = httpParse(file);
		return httpResponse
	}

	return NULL;
}

int isExpired(HttpResponse *response){
	char *expiresDateStr = NULL;
	char *responseDateStr = NULL;
	time_t expiresDate;
	time_t now;

	time(&now);
	expiresDateStr = httpFindHeaderByName(EXPIRES_HEADER);

	if (expiresDateStr != NULL) {
		expiresDate = convertToTime(expiresDateStr, 0);
		if (expiresDate > now) {
			return TRUE;;
		}
	} else {
		responseDateStr = httpFindHeaderByName(DATE_HEADER);
		if (responseDateStr == NULL) {
			return TRUE;;
		}
		expiresDate = convertToTime(responseDateStr, CACHED_RESPONSE_LIFETIME);
		if (responseDate > now) {
			return TRUE;;
		}
	}

	return FALSE;
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

// Retorna a string do hash em base 16.
char *calculateHash(char *request){
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

char *readAsString(char *filename){
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
}

time_t convertToTime(char *dateStr, int minutesToAdd){
	struct tm date;

	strptime(dateStr, "%a, %d %b %Y %H:%M:%S GMT", &date);
	date.tm_min += minutesToAdd; 
	return timegm(&date);
}

