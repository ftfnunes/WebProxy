#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "WebCache.h"
#include "HttpHandler.h"

#define REQUEST "GET /docs/index.html HTTP/1.1\r\n\
Host: www.nowhere123.com\r\n\
Accept: image/gif, image/jpeg, */*\r\n\
Accept-Language: en-us\r\n\
Accept-Encoding: gzip, deflate\r\n\
User-Agent: Chrome/4.0 (compatible; MSIE 6.0; Windows NT 5.1)\r\n\r\n"

#define RESPONSE "HTTP/1.1 200 OK\r\n\
Content-Type: text/xml; charset=utf-8\r\n\
Server: Microsoft-IIS/10.0\r\n\
X-ActivityId: 263ec7cb-cc16-4f86-9509-3629413c2e53\r\n\
Access-Control-Allow-Origin: *\r\n\
X-AspNet-Version: 4.0.30319\r\n\
X-Powered-By: ASP.NET\r\n\
Cache-Control: public, max-age=524\r\n\
Date: Sun, 3 Dec 2017 08:06:00 GMT\r\n\
Expires: Sun, 3 Dec 2017 23:06:00 GMT\r\n\
Content-Length: 4303\r\n\
Connection: keep-alive\r\n\r\n"

#define URI "/docs/index.html"

#define HOST "www.nowhere123.com"

int main() {
    HttpRequest *request;
    HttpResponse *mockResponse;
    HttpResponse *cachedResponse;
    QueueNode *cur;
    int length;
    char *raw;

    initializeCache();

    printf("InitialState\n");
    printf("cache size: %ld\n", cacheSize);
    for (cur = fileQueue->first; cur != NULL; cur = cur->qNext) {
        printf("%s\t", cur->key);
    }
    printf("\n");

    request = (HttpRequest *)malloc(sizeof(HttpRequest));

    request->uri = (char *)malloc((strlen(URI)+1)*sizeof(char));
    strcpy(request->uri, URI);
    request->hostname = (char *)malloc((strlen(HOST)+1)*sizeof(char));
    strcpy(request->hostname, HOST);
    cachedResponse = getResponseFromCache(request);

    if (cachedResponse == NULL) {
        printf("Cache miss\n");
    } else {
        raw = getResponseRaw(cachedResponse, &length);
        printf("Cache HIT!\n");
        fwrite(raw, sizeof(char), length, stdout);
        printf("\n");
        free(raw);
    }

    mockResponse = (HttpResponse *)malloc(sizeof(HttpResponse));
    mockResponse->version = (char *)malloc(10*sizeof(char));
    mockResponse->statusCode = 200;
    mockResponse->reasonPhrase = (char *)malloc(3*sizeof(char));
    strcpy(mockResponse->version, "HTTP/1.1");
    strcpy(mockResponse->reasonPhrase, "OK");

    mockResponse->headers = (HeaderField *)malloc(3*sizeof(HeaderField));
    mockResponse->headerCount = 3;
    mockResponse->bodySize = 0;

    mockResponse->headers[0].name = (char *)malloc(20*sizeof(char));
    mockResponse->headers[0].value = (char *)malloc(50*sizeof(char));
    strcpy(mockResponse->headers[0].name, "Cache-Control");
    strcpy(mockResponse->headers[0].value, "public");

    mockResponse->headers[1].name = (char *)malloc(20*sizeof(char));
    mockResponse->headers[1].value = (char *)malloc(50*sizeof(char));
    strcpy(mockResponse->headers[1].name, "Date");
    strcpy(mockResponse->headers[1].value, "Sun, 5 Dec 2017 00:04:00 GMT");

    mockResponse->headers[2].name = (char *)malloc(20*sizeof(char));
    mockResponse->headers[2].value = (char *)malloc(50*sizeof(char));
    strcpy(mockResponse->headers[2].name, "Expires");
    strcpy(mockResponse->headers[2].value, "Sun, 5 Dec 2017 00:00:00 GMT");

    storeInCache(mockResponse, request);

    cachedResponse = getResponseFromCache(request);
    if (cachedResponse != NULL) {
        raw = getResponseRaw(cachedResponse, &length);
        printf("Cache HIT!\n");
        fwrite(raw, sizeof(char), length, stdout);
        printf("\n");
        free(raw);
    } else {
        printf("response not found\n");
    }

    printf("UpdatedState\n");
    printf("cache size: %ld\n", cacheSize);
    for (cur = fileQueue->first; cur != NULL; cur = cur->qNext) {
        printf("%s\t", cur->key);
    }
    printf("\n");

    printf("\n\nIs expired? %d\n", isExpired(cachedResponse));
    printf("\nShould be cached? %d\n", shouldBeCached(cachedResponse));
    return 0;
}
