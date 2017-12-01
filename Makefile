IDIR =./include
CC=gcc
CFLAGS= -Wall -g -I$(IDIR) -lcrypto

SRC_DIR = ./src

ODIR=obj

BDIR=bin

OBJS= RequestValidator.o WebCache.o HttpHandler.o Common.o Log.o
OBJS_PATH= $(ODIR)/RequestValidator.o $(ODIR)/WebCache.o $(ODIR)/HttpHandler.o $(ODIR)/Common.o

_make_obj := $(shell mkdir -p obj)
_make_bin := $(shell mkdir -p bin)

WebProxy: $(SRC_DIR)/WebProxy.c $(OBJS)
	$(CC) -o $(BDIR)/$@ $(SRC_DIR)/WebProxy.c $(OBJS_PATH) $(CFLAGS)

%.o: $(SRC_DIR)/%.c $(IDIR)/%.h
	$(CC) $(CFLAGS) -c $< -o $(ODIR)/$@

.PHONY: clean

clean:
	rm -f $(BDIR)/* $(ODIR)/*.o *~ core $(INCDIR)/*~
