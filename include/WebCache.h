#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>
#include <unistd.h>
#include "HttpHandler.h"
#include <time.h>

#ifndef WEB_CACHE
#define WEB_CACHE

#define HASH_SIZE SHA256_DIGEST_LENGTH*2+1
#define CACHE_FILENAME_SIZE HASH_SIZE+6
#define EXPIRES_HEADER "Expires"
#define DATE_HEADER "Date"
#define CACHED_RESPONSE_LIFETIME 5 //In minutes

#endif