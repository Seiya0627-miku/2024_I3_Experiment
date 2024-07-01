CC = gcc

BINDIR = bin
INCLUDEDIR = include
SRCDIR = src

CFLAGS = -Wall -I$(INCLUDEDIR)
LDLIBS = -lm

SRCS = $(wildcard $(SRCDIR)/*.c)

OBJS = $(SRCS:%.c=%.o)

SERVER = $(BINDIR)/server
CLIENT = $(BINDIR)/client

all: $(SERVER) $(CLIENT)

$(SERVER): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(filter-out src/telephone_client.o, $(OBJS)) $(LDLIBS)

$(CLIENT): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(filter-out src/telephone_server.o, $(OBJS)) $(LDLIBS)

.PHONY: tmpclean clean

tmpclean:
	rm -f $(SRCDIR)/*~ *~

clean: tmpclean
	rm -f $(TARGET) $(ARRAY_HISTORY) $(OBJS)
	
