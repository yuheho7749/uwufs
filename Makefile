CC = gcc
CFLAGS = -Wall -g

SRC_DIR = src
BUILD_DIR = build

all: $(BUILD_DIR) phase1

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

phase1: $(SRC_DIR)/phase1/spam-mail.c
	$(CC) $(CFLAGS) $^ -l fuse3 -o $@

clean:
	rm -f $(BUILD_DIR)/*.o phase1
