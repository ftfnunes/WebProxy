#include "RequestValidator.h"

/*Essa funcao retorna uma List alocada dinamicamente, nao esquecer do freeList(...)!!*/
List* createList(){
	List* list;
	list = (List*)malloc(sizeof(List));
	list->firstNode = NULL;
	list->lastNode = NULL;

	return list;
}

void freeList(List* list){
	if(!list) return;

	Node* nodeAux;

	while(list->firstNode){
		nodeAux = list->firstNode;
		free(nodeAux->string);
		list->firstNode = nodeAux->next;
		free(nodeAux);
	}

	free(list);
}

void addToList(List* list, char newString[]){
	Node* newNode;

	if(!list){
		printf("A List nao pode ser NULL!\n");
		exit(1);
	}

	newNode = (Node*)malloc(sizeof(Node));
	newNode->string = (char *)malloc(strlen(newString)*sizeof(char));
	strcpy(newNode->string, newString);
	newNode->next = NULL;

	if(!list->firstNode){
		list->firstNode = newNode;
		list->lastNode = newNode;
		return;
	}

	list->lastNode->next = newNode;
	list->lastNode = newNode;
}

List* getList(char fileName[]){
	FILE *fp;
	char stringBuffer[MAX_URL_SIZE];
	List *list = NULL;

	fp = fopen(fileName, "r");

	if(!fp){
		printf("Nao foi possivel abrir o arquivo %s\n", fileName);
		exit(1);
	}

	list = createList();

	while(!feof(fp)){
		if(fscanf(fp, "%s", stringBuffer) > 0){
			addToList(list, stringBuffer);
		}
	}

	fclose(fp);

	return list;
}

void printList(List* list){
	if(!list){
		printf("A List nao pode ser NULL!\n");
		exit(1);
	}

	Node* nodeAux = list->firstNode;

	while(nodeAux){
		printf("%s\n", nodeAux->string);
		nodeAux = nodeAux->next;
	}
}

/*Essa funcao retorna uma string alocada dinamicamente, nao esquecer do free(...)!!*/
char* toLowerCase(char string[], int stringSize){
	int i;
	char *lowerCaseString = (char *)malloc(stringSize*sizeof(char));

	for(i = 0; i < stringSize; i++){
		lowerCaseString[i] = tolower(string[i]);
	}

	return lowerCaseString;
}

void toLowerDenyTerms(List* list){
	if(!list){
		printf("A List de deny terms nao pode ser NULL!\n");
		exit(1);
	}

	Node* nodeAux = list->firstNode;

	while(nodeAux){
		nodeAux->string = toLowerCase(nodeAux->string, strlen(nodeAux->string));
		nodeAux = nodeAux->next;
	}
}

/*Essa funcao retorna uma string com memoria alocada dinamicamente, nao esquecer do free(...)!!*/
/*Essa funcao deve ser usada SOMENTE para Blacklist ou Whitelist. Para DenyList, use a isOnDenyList(...)!!*/
char* isOnList(List* list, char scannedString[]){ 
	if(!list || !scannedString){
		printf("Nem a List e nem o Hostname podem ser NULL!\n");
		exit(1);
	}

	Node* nodeAux = list->firstNode;
	char *foundString = NULL;
	char *lowerScannedString = toLowerCase(scannedString, strlen(scannedString));

	while(nodeAux){
		if(strstr(lowerScannedString, nodeAux->string)){
			foundString = (char *)malloc(strlen(nodeAux->string)*sizeof(char));
			strcpy(foundString, nodeAux->string);
			return foundString;
		}
		nodeAux = nodeAux->next;
	}

	free(lowerScannedString);

	return foundString;
}

/*Essa funcao retorna uma string com memoria alocada dinamicamente, nao esquecer do free(...)!!*/
/*Essa funcao deve ser usada SOMENTE para DenyList. Para Blacklist ou Whitelist, use a isOnList(...)!!*/
/*Diferente da isOnList, essa funcao usa nossa implementacao de strstr e recebe o tamanho do body, para resolver problemas relativos a \0's dentro deste*/
char* isOnDenyList(List* denyList, char body[], int bodySize){ 
	if(!denyList){
		printf("A List nao pode ser NULL!\n");
		exit(1);
	}

	if(!body) return NULL;

	Node* nodeAux = denyList->firstNode;
	char *foundString = NULL;
	char *lowerBody = toLowerCase(body, bodySize);

	while(nodeAux){
		if(isSubstring(lowerBody, nodeAux->string, bodySize)){
			foundString = (char *)malloc(strlen(nodeAux->string)*sizeof(char));
			strcpy(foundString, nodeAux->string);
			return foundString;
		}
		nodeAux = nodeAux->next;
	}

	free(lowerBody);

	return foundString;
}

/*Essa funcao retorna um ValidationResult com uma string alocados dinamicamente, nao esquecer do freeValidationResult(...)!!*/
ValidationResult* ValidateRequest(char *hostname, char *body, int bodySize, List *whiteList, List *blackList, List *denyTerms){
	ValidationResult *vResult;
	char *foundString = NULL;

	vResult = (ValidationResult *)malloc(sizeof(ValidationResult));

	vResult->isOnWhitelist = FALSE;
	vResult->isOnBlacklist = FALSE;
	vResult->isOnDeniedTerms = FALSE;
	vResult->deniedTerm = NULL;

	foundString = isOnList(whiteList, hostname);
	if(foundString){
		vResult->isOnWhitelist = TRUE;
		free(foundString);
		return vResult;
	}

	foundString = isOnList(blackList, hostname);
	if(foundString){
		vResult->isOnBlacklist = TRUE;
		free(foundString);
		return vResult;
	}

	foundString = isOnDenyList(denyTerms, body, bodySize);
	if(foundString){
		vResult->isOnDeniedTerms = TRUE;
		vResult->deniedTerm = (char *)malloc(strlen(foundString)*sizeof(char));
		strcpy(vResult->deniedTerm, foundString);
		free(foundString);
		return vResult;
	}

	return vResult;
}

/*Essa funcao retorna um ValidationResult com uma string alocados dinamicamente, nao esquecer do free dos 2!!*/
ValidationResult* ValidateResponse(char *body, int bodySize, List *denyTerms){
	ValidationResult *vResult;
	char *foundString = NULL;

	vResult = (ValidationResult *)malloc(sizeof(ValidationResult));

	vResult->isOnWhitelist = FALSE;
	vResult->isOnBlacklist = FALSE;

	foundString = isOnDenyList(denyTerms, body, bodySize);
	if(foundString){
		vResult->isOnDeniedTerms = TRUE;
		vResult->deniedTerm = (char *)malloc(strlen(foundString)*sizeof(char));
		strcpy(vResult->deniedTerm, foundString);
		free(foundString);
		return vResult;
	}

	vResult->isOnDeniedTerms = FALSE;
	vResult->deniedTerm = NULL;

	return vResult;
}

void freeValidationResult(ValidationResult *vr){
	if(!vr) return;

	if(vr->deniedTerm) free(vr->deniedTerm);

	free(vr);
}