VERSION = 0
SUBVERSION = 2
PATCH = 17
EXTRAVERSION = -a

DEBUG = 1
FUSE_USE_VERSION = 31

CC = gcc
CFLAGS = -Wall -Wno-unused

ifeq ($(DEBUG), 1)
	CFLAGS += -g -DDEBUG
endif

SRC_DIR = src
BUILD_DIR = build


COMMON_FILES = $(SRC_DIR)/uwufs/uwufs.h $(SRC_DIR)/uwufs/low_level_operations.h $(SRC_DIR)/uwufs/low_level_operations.c $(SRC_DIR)/uwufs/file_operations.h $(SRC_DIR)/uwufs/file_operations.c

all: $(BUILD_DIR) mkfs.uwu mount.uwu test

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

# C++
CXX = g++

cpp_inode_tests: $(COMMON_FILES) $(SRC_DIR)/test/cpp_inode_tests.cpp
	$(CXX) $(CFLAGS) $^ -lfuse3 -o $@

CPP_SRC_DIR = $(SRC_DIR)/uwufs/cpp
CPP_COMMON_FILES = $(CPP_SRC_DIR)/c_api.cpp $(CPP_SRC_DIR)/DataBlockIterator.cpp $(CPP_SRC_DIR)/INode.cpp

$(CPP_SRC_DIR)/c_api.o: $(CPP_SRC_DIR)/c_api.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

$(CPP_SRC_DIR)/DataBlockIterator.o: $(CPP_SRC_DIR)/DataBlockIterator.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

$(CPP_SRC_DIR)/INode.o: $(CPP_SRC_DIR)/INode.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

c_api_test: $(COMMON_FILES) $(SRC_DIR)/test/c_api_test.c $(CPP_SRC_DIR)/c_api.o $(CPP_SRC_DIR)/DataBlockIterator.o $(CPP_SRC_DIR)/INode.o
	$(CXX) $(CFLAGS) $^ -lfuse3 -o $@

clean:
	rm -f $(BUILD_DIR)/*.o phase1 mkfs.uwu test mount.uwu
