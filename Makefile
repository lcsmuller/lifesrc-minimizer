CC = g++

SRC_DIR              = src
MERGESAT_DIR         = mergesat
MERGESAT_BUILD_DIR   = $(MERGESAT_DIR)/build/release
MERGESAT_LIB_DIR     = $(MERGESAT_BUILD_DIR)/lib
MERGESAT_INCLUDE_DIR = $(MERGESAT_DIR)/minisat

LIBMERGESAT          = $(MERGESAT_LIB_DIR)/libmergesat.a

OBJS    = $(SRC_DIR)/commandline.o \
		  $(SRC_DIR)/field.o       \
		  $(SRC_DIR)/formula.o     \
		  $(SRC_DIR)/pattern.o
MAIN    = gol-sat

CFLAGS  = -std=c++98 -I$(SRC_DIR) -I$(MERGESAT_INCLUDE_DIR)
LDFLAGS = -L$(MERGESAT_LIB_DIR)
LDLIBS  = -lmergesat -lpthread

all: $(MAIN)

$(MAIN): $(OBJS)

$(OBJS): $(LIBMERGESAT)

$(LIBMERGESAT):
	@ echo "Initializing mergesat submodule..."
	git submodule update --init --recursive
	@ $(MAKE) -C $(MERGESAT_DIR)

clean:
	@ $(MAKE) -C $(SRC_DIR) $@
	@ rm -f $(MAIN)

purge: clean
	git submodule deinit -f $(MERGESAT_DIR)

.PHONY: all clean purge
