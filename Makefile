IDIR =./include
CC=gcc
CFLAGS= -Wall -g -I$(IDIR) -lcrypto

SRC_DIR = ./src

CACHE_TESTS_DIR = ./tests/cache
VALIDATOR_TESTS_DIR = ./tests/validator

ODIR=obj

BDIR=bin

OBJS= RequestValidator.o WebCache.o HttpHandler.o Common.o Log.o CustomQueue.o
OBJS_PATH= $(ODIR)/RequestValidator.o $(ODIR)/WebCache.o $(ODIR)/HttpHandler.o $(ODIR)/Common.o $(ODIR)/Log.o $(ODIR)/CustomQueue.o

_make_obj := $(shell mkdir -p obj)
_make_bin := $(shell mkdir -p bin)
_make_cache := $(shell mkdir -p ./$(BDIR)/cache)

WebProxy: $(SRC_DIR)/WebProxy.c $(OBJS)
	$(CC) -o $(BDIR)/$@ $(SRC_DIR)/WebProxy.c $(OBJS_PATH) $(CFLAGS)

%.o: $(SRC_DIR)/%.c $(IDIR)/%.h
	$(CC) $(CFLAGS) -c $< -o $(ODIR)/$@

CacheTests: $(CACHE_TESTS_DIR)/cacheTests.c $(OBJS)
		$(CC) -o $(BDIR)/$@ $(CACHE_TESTS_DIR)/cacheTests.c $(OBJS_PATH) $(CFLAGS)

ValidatorTests: $(VALIDATOR_TESTS_DIR)/RequestValidatorTests.c $(OBJS)
		$(CC) -o $(BDIR)/$@ $(VALIDATOR_TESTS_DIR)/RequestValidatorTests.c $(OBJS_PATH) $(CFLAGS)


.PHONY: clean

clean:
	rm -f $(BDIR)/* $(ODIR)/*.o *~ core $(INCDIR)/*~
