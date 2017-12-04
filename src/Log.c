#include "Log.h"

void logError(char *errorMessage) {
	logMessage(errorMessage, ERROR);
}

void logSuccess(char *successMessage) {
	logMessage(successMessage, SUCCESS);
}

void logWarning(char *warningMessage) {
	logMessage(warningMessage, WARNING);
}

void logMessage(char *message, int messageType) {
	char *logMessage = (char *)malloc((LOG_HEADER_LENGTH*strlen(message) + 1)*sizeof(char));
	char date[LOG_HEADER_LENGTH];
	char header[LOG_HEADER_LENGTH];

	getDateString(date);

	switch (messageType) {
		case SUCCESS:
			strcpy(header, "SUCCESS");
			break;
		case ERROR:
			strcpy(header, "ERROR");
			break;
		default:
			strcpy(header, "WARNING");
			break;
	}

	sprintf(logMessage, "%s - %s - %s", header, date, message);

	pthread_mutex_lock(&logMutex);
	writeLog(logMessage);
	pthread_mutex_unlock(&logMutex);
	free(logMessage);
}

void writeLog(char *logMessage) {
	FILE *fp = fopen(LOG_FILE, "a");

	if (fp == NULL) {
		printf("Error happened while opening log file - %s", logMessage);
		return;
	}

	fprintf(fp, "\n%s\n", logMessage);
	fclose(fp);
}

void getDateString(char date[]) {
	time_t now = time(NULL);
	strftime(date, LOG_HEADER_LENGTH, "%Y-%m-%d %H:%M:%S", gmtime(&now));
}

void initializeLog() {
	pthread_mutex_init(&logMutex, NULL);
}
