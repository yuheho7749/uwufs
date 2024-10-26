VERSION = 0
SUBVERSION = 2
PATCH = 5
EXTRAVERSION = -a

DEBUG = 0

CC = g++
CFLAGS = -Wall

ifeq ($(DEBUG), 1)
	CFLAGS += -g -DDEBUG
endif

SRC_DIR = src
BUILD_DIR = build

all: $(BUILD_DIR) phase1 mkfs.uwu

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

phase1: $(SRC_DIR)/phase1/spam-mail.c
	$(CC) $(CFLAGS) $^ -lfuse3 -o $@

mkfs.uwu: $(SRC_DIR)/uwufs/*.c
	$(CC) $(CFLAGS) $^ -lfuse3 -o $@

test: $(SRC_DIR)/test/test.c $(SRC_DIR)/uwufs/low_level_operations.* $(SRC_DIR)/uwufs/uwufs.h
	$(CC) $(CFLAGS) $^ -lfuse3 -o $@

clean:
	rm -f $(BUILD_DIR)/*.o phase1 mkfs.uwu test
