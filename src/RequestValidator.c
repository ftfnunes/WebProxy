#include "RequestValidator.h"



//Esse funcao instancia e retorna uma estrutura de lista, que pode ser usada tanto para a blacklist, 
//quanto para a whitelist ou para a lista de denied terms
List* createList(){
	List* list;
	list = (List*)malloc(sizeof(List));
	list->firstNode = NULL;
	list->lastNode = NULL;

	return list;
}

//Essa funcao libera uma lista e todos recursos (nodes) que ela contem
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

//Essa funcao tem como objetivo adicionar uma nova string a uma lista.
//Essa string virara um node dentro da lista.
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

//Essa funcao tem como objetivo varrer um arquivo (blacklist/whitelist/denyList) e colocar 
//todos os elementos desse arquivo numa lista (List) a qual a propria funcao instancia e retorna
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

//Imprime todos os elementos de uma determinada lista
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


//Essa funcao recebe uma string, e o tamanho dela, e tem como objetivo
//tranformar todos os caracteres da mesma em lower case.
//(A funcao retorna a string transformada, deixando a string original intacta)
char* toLowerCase(char string[], int stringSize){
	int i;
	char *lowerCaseString = (char *)malloc(stringSize*sizeof(char));

	for(i = 0; i < stringSize; i++){
		lowerCaseString[i] = tolower(string[i]);
	}

	return lowerCaseString;
}

//Transforma todos os elementos de uma lista de denied terms em lower case
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

//Essa funcao tem como objetivo verificar se a string escaneada possui, como substring, algum dos elementos presentes na lista.
//Caso um elemento seja encontrado na scanned string, ele eh retornado.
//(Essa funcao deve ser usada SOMENTE para Blacklist ou Whitelist. Para DenyList, use a isOnDenyList(...)!!)
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


//Diferente da isOnList, essa funcao usa nossa implementacao de strstr e recebe o tamanho do body, para resolver problemas relativos a \0's dentro deste.
//Tirando isso, ela tem o mesmo objetivo da isOnList, funcionando de forma quase identica.
//(Essa funcao deve ser usada SOMENTE para DenyList. Para Blacklist ou Whitelist, use a isOnList(...)!!)
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

//(FUNCAO PRINCIPAL DESTE MODULO)
//Essa funcao tem como objetivo fazer todas as verificacoes de whitelist, blacklist e denied terms, baseado no hostname, body e tamanho do body da REQUEST.
//Ela retorna um ValidationResult, que eh uma struct que contem os resultados das verificacoes
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

//(FUNCAO PRINCIPAL DESTE MODULO)
//Essa funcao tem como objetivo fazer as verificacoes de denied terms, baseado no body e no tamanho do body da RESPONSE.
//Ela retorna um ValidationResult, que eh uma struct que contem os resultados das verificacoes
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