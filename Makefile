IDIR =./include
CC=gcc
CFLAGS= -Wall -g -I$(IDIR)

SRC_DIR = ./src

ODIR=obj

BDIR=bin

OBJS= RequestValidator.o WebCache.o HttpHandler.o
OBJS_PATH= obj/RequestValidator.o obj/WebCache.o obj/HttpHandler.o

_make_obj := $(shell mkdir -p obj)
_make_bin := $(shell mkdir -p bin)

WebProxy: $(OBJS) $(SRC_DIR)/WebProxy.c 
	$(CC) -o $(BDIR)/$@ $(SRC_DIR)/WebProxy.c $(OBJS_PATH) $(CFLAGS)

%.o: $(SRC_DIR)/%.c $(IDIR)/%.h
	$(CC) $(CFLAGS) -c $< -o $(ODIR)/$@

.PHONY: clean

clean:
	rm -f $(BDIR)/* $(ODIR)/*.o *~ core $(INCDIR)/*~ 