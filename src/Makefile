TOP = ..
CC  = g++

OBJS = commandline.o \
       field.o       \
       formula.o     \
       pattern.o

MERGESAT_DIR         = $(TOP)/mergesat
MERGESAT_INCLUDE_DIR = $(MERGESAT_DIR)/minisat

CFLAGS = -std=c++98 -I. -I$(MERGESAT_INCLUDE_DIR)

all: $(OBJS)

clean:
	@ rm -f $(OBJS)

.PHONY: all clean
