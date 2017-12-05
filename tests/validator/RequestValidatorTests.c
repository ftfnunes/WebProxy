#include "RequestValidator.h"

int main(){
	/*TESTE DA FUNCAO TOLOWERCASE*/
	
	/*char body[] = "Em linguística, a noção de texto é ampla e ainda aberta a uma definição mais precisa. Grosso modo, pode ser entendido como manifestação linguística das ideias de um autor, que serão interpretadas pelo leitor de acordo com seus conhecimentos linguísticos e culturais. Seu tamanho é variável.";

	char *lower = toLowerCase(body);

	printf("%s\n", body);
	printf("\n%s\n", lower);

	free(lower);

	return 0;*/
	

	/*TESTE DA ISONDENYLIST*/

	/*char fileDeny[] = "deny.txt";
	List *denyList;

	denyList = getList(fileDeny);

	char body[] = "kjhiovsjtortksanjd\0aisgrossocnas\0\0sdfj";

	printf("Esta na deny list? %s\n", isOnDenyList(denyList, body, 38));

	freeList(denyList);*/
	

	/*TESTE GERAL: testa o getList (que utiliza o createList e o addToList), o printList, o freeList e os Validates (que usam o isOnList)*/

	char fileWhitelist[] = "whitelist.txt";
	char fileBlacklist[] = "blacklist.txt";
	char fileDeny[] = "denyTerms.txt";
	List *whitelist, *blackList, *denyList;
	ValidationResult *RequestVR, *ResponseVR;

	char hostname[] = "youtube.com";
	char body[] = "kjhiovsjtortksanjd\0aisgrossocnas\0\0sdfj";

	whitelist = getList(fileWhitelist);
	blackList = getList(fileBlacklist);
	denyList = getList(fileDeny);
	toLowerDenyTerms(denyList);

	printf("Imprimindo %s:\n", fileWhitelist);
	printList(whitelist);
	printf("\nImprimindo %s:\n", fileBlacklist);
	printList(blackList);
	printf("\nImprimindo %s:\n", fileDeny);
	printList(denyList);

	printf("\n");

	RequestVR = ValidateRequest(hostname, body, 38, whitelist, blackList, denyList);

	printf("Request Validation Result:\n");
	printf("-isOnWhitelist: %s;\n", (RequestVR->isOnWhitelist)?"true":"false");
	printf("-isOnBlacklist: %s;\n", (RequestVR->isOnBlacklist)?"true":"false");
	printf("-isOnDeniedTerms: %s.\n", (RequestVR->isOnDeniedTerms)?"true":"false");

	ResponseVR = ValidateResponse(body, 38, denyList);

	printf("\nResponse Validation Result:\n");
	printf("-isOnWhitelist: %s;\n", (ResponseVR->isOnWhitelist)?"true":"false");
	printf("-isOnBlacklist: %s;\n", (ResponseVR->isOnBlacklist)?"true":"false");
	printf("-isOnDeniedTerms: %s.\n", (ResponseVR->isOnDeniedTerms)?"true":"false");

	freeValidationResult(RequestVR);
	freeValidationResult(ResponseVR);
	freeList(whitelist);
	freeList(blackList);
	freeList(denyList);

	return 0;
}