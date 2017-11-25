#include <stdio.h>
#include <stdlib.h>

typedef struct validationResult{
	short isOnBlacklist;
	short isOnWhitelist;
	short isOnDeniedTerm;
	char *deniedTerm;
} ValidationResult;