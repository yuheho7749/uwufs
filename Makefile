VERSION = 0
SUBVERSION = 2
PATCH = 16
EXTRAVERSION = -a

DEBUG = 0
FUSE_USE_VERSION = 31

CC = g++
CFLAGS = -Wall

ifeq ($(DEBUG), 1)
	CFLAGS += -g -DDEBUG
endif

SRC_DIR = src
BUILD_DIR = build

COMMON_FILES = $(SRC_DIR)/uwufs/uwufs.h $(SRC_DIR)/uwufs/low_level_operations.h $(SRC_DIR)/uwufs/low_level_operations.c $(SRC_DIR)/uwufs/file_operations.h $(SRC_DIR)/uwufs/file_operations.c

all: $(BUILD_DIR) phase1 mkfs.uwu mount.uwu

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

phase1: $(SRC_DIR)/phase1/spam-mail.c
	$(CC) $(CFLAGS) $^ -lfuse3 -o $@

mkfs.uwu: $(COMMON_FILES) $(SRC_DIR)/uwufs/mkfs_uwu.c
	$(CC) $(CFLAGS) $^ -lfuse3 -o $@

mount.uwu: $(COMMON_FILES) $(SRC_DIR)/uwufs/mount_uwufs.c $(SRC_DIR)/uwufs/syscalls.h $(SRC_DIR)/uwufs/syscalls.c
	$(CC) $(CFLAGS) $^ -lfuse3 -o $@

test: $(COMMON_FILES) $(SRC_DIR)/test/test.c
	$(CC) $(CFLAGS) $^ -lfuse3 -o $@

clean:
	rm -f $(BUILD_DIR)/*.o phase1 mkfs.uwu test mount.uwu
