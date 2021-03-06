#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>

#ifndef COMMON_INC

#define COMMON_INC

#define MAX_N_OF_CONNECTIONS 5
#define MAX_URL_SIZE 2048 /*baseado no tamanho maximo de url do IE*/
#define TRUE 1
#define FALSE 0
#define HASH_SIZE SHA256_DIGEST_LENGTH*2+1

typedef struct node {
	char *string;
	struct node *next;
} Node;

typedef struct {
	Node *firstNode;
	Node *lastNode;
} List;

struct socket_str {
	struct sockaddr_in* sockAddr;
	int socket;
	short inspect;
	List *whitelist;
	List *blacklist;
	List *denyTerms;
};


typedef struct socket_str ThreadContext;

void configureSockAddr(struct sockaddr_in* sockAddr, int port, unsigned long addr);
int stringCopy(char *dest, char *src, int srcSize);
char *isSubstring(char *scannedString, char *matchString, int scannedStringSize);

#endif
