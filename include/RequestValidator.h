#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> /*tolower()*/
#include "Common.h" /*Constantes como MAX_URL_SIZE, TRUE e FALSE*/

#ifndef REQUEST_VALIDATOR

#define REQUEST_VALIDATOR

//Essa struct contem os resultados das verificacoes de whitelist, blacklist 
//e denied terms, que sao feitas nas requests/responses
typedef struct validationResult{
	short isOnBlacklist;
	short isOnWhitelist;
	short isOnDeniedTerms;
	char *deniedTerm;
} ValidationResult;

List* createList(); /*Retorna um (List *)malloc, nao esquecer do freeList(...)!*/
void freeList(List* list);
void addToList(List* list, char newString[]);
List* getList(char fileName[]);
void printList(List* list);
char* toLowerCase(char string[], int stringSize); /*Retorna um (char *)malloc, nao esquecer do free(...)!*/
void toLowerDenyTerms(List* list);
char* isOnList(List* list, char scannedString[]); /*Retorna um (char *)malloc, nao esquecer do free(...)!*/
char* isOnDenyList(List* denyList, char body[], int bodySize); /*Retorna um (char *)malloc, nao esquecer do free(...)!*/
ValidationResult* ValidateRequest(char *hostname, char *body, int bodySize, List *whiteList, List *blackList, List *denyTerms); /*Retorna um (ValidationResult *)malloc, nao esquecer do freeValidationResult(...)!*/
ValidationResult* ValidateResponse(char *body, int bodySize, List *denyTerms); /*Retorna um (ValidationResult *)malloc, nao esquecer do freeValidationResult(...)!*/
void freeValidationResult(ValidationResult *vr);

#endif