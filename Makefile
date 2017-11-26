IDIR =./include
CC=gcc
CFLAGS= -Wall -g -I$(IDIR) -lcrypto

SRC_DIR = ./src

ODIR=obj

BDIR=bin

OBJS= RequestValidator.o WebCache.o HttpHandler.o
OBJS_PATH= $(ODIR)/RequestValidator.o $(ODIR)/WebCache.o $(ODIR)/HttpHandler.o
HEADERS= $(IDIR)/RequestValidator.h $(IDIR)/WebCache.h $(IDIR)/HttpHandler.h $(IDIR)/Common.h

_make_obj := $(shell mkdir -p obj)
_make_bin := $(shell mkdir -p bin)

WebProxy: $(SRC_DIR)/WebProxy.c $(OBJS_PATH) $(HEADERS)
	$(CC) -o $(BDIR)/$@ $(SRC_DIR)/WebProxy.c $(OBJS_PATH) $(CFLAGS)

$(ODIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -f $(BDIR)/* $(ODIR)/*.o *~ core $(INCDIR)/*~ 