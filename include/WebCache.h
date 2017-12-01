#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include "HttpHandler.h"
#include "Common.h"
#include "Log.h"

#ifndef WEB_CACHE
#define WEB_CACHE

#define HASH_SIZE SHA256_DIGEST_LENGTH*2+1
#define CACHE_FILENAME_SIZE HASH_SIZE+6
#define EXPIRES_HEADER "Expires"
#define DATE_HEADER "Date"
#define CACHED_RESPONSE_LIFETIME 5 //In minutes
#define SIZE_OF_MESSAGE 40

pthread_mutex_t cacheMutex;

HttpResponse *getResponseFromCache(HttpRequest* request);

int isExpired(HttpResponse *response);

void storeInCache(HttpResponse *response, HttpRequest* request);

char convertToHexa(unsigned char c);

char *calculateHash(char *request);

char *readAsString(char *filename);

time_t convertToTime(char *dateStr, int minutesToAdd);

char *findHeaderByName(char *name, HeaderField *headers, int headerCount);

void initializeCache();

#endif
