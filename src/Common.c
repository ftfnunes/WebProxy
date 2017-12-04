#include "Common.h"

void configureSockAddr(struct sockaddr_in* sockAddr, int port, unsigned long addr) {
	bzero(sockAddr, sizeof(struct sockaddr_in));
	sockAddr->sin_family = AF_INET;
	sockAddr->sin_port = htons(port);
	sockAddr->sin_addr.s_addr = addr;
	bzero(&(sockAddr->sin_zero), 8);
}

int stringCopy(char *dest, char *src, int srcSize){
	int i;

	for (i = 0; i < srcSize; ++i){
		dest[i] = src[i];
	}

	return i;
}

char *isSubstring(char *scannedString, char *matchString, int scannedStringSize){
	int pivot = 0, isStringFound = 1, i;
	int matchStringSize = (int) strlen(matchString);

	while((pivot + matchStringSize - 1) < scannedStringSize){
		isStringFound = 1;
		for(i = 0; i < matchStringSize; i++){
			if(scannedString[pivot + i] != matchString[i]){
				isStringFound = 0;
				break;
			}
		}
		if(isStringFound) return &scannedString[pivot];

		pivot++;
	}

	return NULL;
}