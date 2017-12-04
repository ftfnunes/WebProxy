#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "HttpHandler.h"
#include "Common.h"
#include "Log.h"
#include "CustomQueue.h"

#ifndef WEB_CACHE
#define WEB_CACHE

#define CACHE_FILENAME_SIZE HASH_SIZE+7
#define EXPIRES_HEADER "Expires"
#define DATE_HEADER "Date"
#define CACHE_CONTROL_HEADER "Cache-Control"
#define PRIVATE_DIRECTIVE "private"
#define NO_CACHE_DIRECTIVE "no-cache"
#define CACHED_RESPONSE_LIFETIME 40 //In minutes
#define SIZE_OF_MESSAGE 500
// TODO: COLOCAR PASTA NO MAKEFILE
#define CACHE_PATH "./cache" //Nao deve terminar com /
#define MAX_CACHE_SIZE 1000000

pthread_mutex_t cacheMutex;
pthread_mutex_t queueMutex;

Queue *fileQueue;

off_t cacheSize;

struct fileListNode {
    char *key;
    time_t lastAccess;
    struct fileListNode *next;
};

typedef struct fileListNode FileListNode;

typedef struct fileList {
    FileListNode *start;
} FileList;

HttpResponse *getResponseFromCache(HttpRequest* request);

int isExpired(HttpResponse *response);

void storeInCache(HttpResponse *response, HttpRequest* request);

char convertToHexa(unsigned char c);

char *calculateHash(char *request);

char *readAsString(char *filename);

time_t convertToTime(char *dateStr, int minutesToAdd);

char *findHeaderByName(char *name, HeaderField *headers, int headerCount);

void initializeCache();

char *getRequestFilename(HttpRequest *request);

void getKeyFromFilename(char *key, char *filename);

int removeLRAResponse();

off_t getCacheStatus(Queue *queue);

char *getFilenameFromKey(char *key);

FileListNode *createFileListNode(char *key, time_t lastAccess);

FileList *createFileList();

void freeFilesList(FileList *list);

void fileListToQueue(FileList *list, Queue *queue);

void insertInOrder(FileList *list, char *key, time_t lastAccess);

char *getUrl(HttpRequest *request);

int shouldBeCached(HttpResponse *response);

char *getResponseRaw(HttpResponse *response, int *length);

#endif
