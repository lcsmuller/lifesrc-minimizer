CC = cc

SRC_DIR             = src
LIFESRC_VERSION     = 3.8+lcsmuller1
LIFESRC_DIR         = lifesrc-$(LIFESRC_VERSION)
LIFESRC_INCLUDE_DIR = $(LIFESRC_DIR)

OBJS       = $(SRC_DIR)/commandline.o \
             $(SRC_DIR)/pattern.o     \
             $(SRC_DIR)/popen2.o
LIFESRC    = lifesrc
MAIN       = gol-sat

CFLAGS  = -std=c89 -D_POSIX_C_SOURCE=200809L -I$(SRC_DIR) \
          -I$(LIFESRC_INCLUDE_DIR) -Wall -Wextra -Wpedantic
LDLIBS  = -lm

all: $(MAIN)

debug:
	@ $(MAKE) CFLAGS="$(CFLAGS) -g -DDEBUG_MODE" $(MAIN)

$(MAIN): $(OBJS)

$(OBJS): $(LIFESRC)

$(LIFESRC):
	@ echo "Building lifesrc library..."
	@ $(MAKE) -C $(LIFESRC_DIR) lifesrcdumb CFLAGS=
	@ cp $(LIFESRC_DIR)/lifesrcdumb ./$(LIFESRC)

clean:
	@ $(MAKE) -C $(SRC_DIR) $@
	@ rm -f $(MAIN) $(LIFESRC)

purge: clean
	$(MAKE) -C $(LIFESRC_DIR) clean

.PHONY: all clean purge
