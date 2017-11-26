#include <stdio.h>
#include <stdlib.h>

#ifndef REQUEST_VALIDATOR

#define REQUEST_VALIDATOR

typedef struct validationResult{
	short isOnBlacklist;
	short isOnWhitelist;
	short isOnDeniedTerms;
	char *deniedTerm;
} ValidationResult;

#endif