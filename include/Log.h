#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#ifndef LOG_INC

#define LOG_INC

#define LOG_FILE "WebProxy.log"
#define LOG_HEADER_LENGTH 50
#define ERROR 0
#define SUCCESS 1
#define WARNING 2

pthread_mutex_t logMutex;

void logError(char *errorMessage);

void logSuccess(char *successMessage);

void logWarning(char *warningMessage);

void logMessage(char *message, int messageType);

void writeLog(char *logMessage);

void getDateString(char date[]);

void initializeLog();

#endif
