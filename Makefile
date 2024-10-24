VERSION = 0
SUBVERSION = 1
PATCH = 5
EXTRAVERSION = -rc1

DEBUG = 0

CC = gcc
CFLAGS = -Wall

ifeq ($(DEBUG), 1)
	CFLAGS += -g -DDEBUG
endif

SRC_DIR = src
BUILD_DIR = build

all: $(BUILD_DIR) phase1 phase2

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

phase1: $(SRC_DIR)/phase1/spam-mail.c
	$(CC) $(CFLAGS) $^ -lfuse3 -o $@

phase2: mkfs.uwu

mkfs.uwu: $(SRC_DIR)/uwufs/mkfs-uwu.c
	$(CC) $(CFLAGS) $^ -lfuse3 -o mkfs.uwu

clean:
	rm -f $(BUILD_DIR)/*.o phase1 mkfs.uwu
