CC = cc

OBJS = commandline.o pattern.o popen2.o

CFLAGS = -std=c89 -I. -Wall -Wextra -Wpedantic

all: $(OBJS)

clean:
	@ rm -f $(OBJS)

.PHONY: all clean
