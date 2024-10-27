VERSION = 0
SUBVERSION = 2
PATCH = 7
EXTRAVERSION = -a

DEBUG = 0

CC = g++
CFLAGS = -Wall

ifeq ($(DEBUG), 1)
	CFLAGS += -g -DDEBUG
endif

# TODO: not building in BUILD_DIR, fix later
SRC_DIR = src
BUILD_DIR = build

COMMON_FILES = $(SRC_DIR)/uwufs/uwufs.h $(SRC_DIR)/uwufs/low_level_operations.h $(SRC_DIR)/uwufs/low_level_operations.c

EXECUTABLES = phase1 mkfs.uwu
TESTS = test test_inode

all: $(BUILD_DIR) $(EXECUTABLES)
tests: $(BUILD_DIR) $(TESTS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

phase1: $(SRC_DIR)/phase1/spam-mail.c
	$(CC) $(CFLAGS) $^ -lfuse3 -o $@

mkfs.uwu: $(COMMON_FILES) $(SRC_DIR)/uwufs/mkfs_uwu.c
	$(CC) $(CFLAGS) $^ -lfuse3 -o $@

test: $(COMMON_FILES) $(SRC_DIR)/test/test.c
	$(CC) $(CFLAGS) $^ -lfuse3 -o $@

test_inode: $(COMMON_FILES) $(SRC_DIR)/test/find_free_inode.c 
	$(CC) $(CFLAGS) $^ -lfuse3 -o $@

clean:
	rm -f $(BUILD_DIR)/*.o $(EXECUTABLES) $(TESTS)
