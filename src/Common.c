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