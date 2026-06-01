CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11 -Iinclude -Isrc/core -Isrc/buffer -Isrc/ui -Isrc/editor -D_XOPEN_SOURCE_EXTENDED
SRC_DIR = src
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/objs

# Find all .c files in SRC_DIR and its subdirectories
SRC = $(shell find $(SRC_DIR) -name "*.c" ! -name "ui_help_overlay.c")
# Map source files to object files in OBJ_DIR, mirroring the structure
OBJ = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
TARGET = $(BUILD_DIR)/stitch

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) -lncursesw

# Pattern rule for object files, creating directories as needed
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
