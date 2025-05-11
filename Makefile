TARGET = reverse_proxy

CC = gcc
DEP_FLAGS = -MP -MD
# C_FLAGS = -std=c11 -Wall -Werror -Wextra -march=native -O2
C_FLAGS = -std=c11 -Wall -Wextra -march=native -O2

BUILD_DIR = build
SRC_DIR = src

C_FILES = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/**/*.c)
H_FILES = $(wildcard $(SRC_DIR)/*.h) $(wildcard $(SRC_DIR)/**/*.h)

OBJ_FILES = $(patsubst %.c, %.o, $(C_FILES))

DEP_FILES = $(patsubst %.c, %.d, $(C_FILES))

all: directories $(BUILD_DIR)/$(TARGET)

directories: $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)/$(TARGET) $(OBJ_FILES) $(TEST_BINS) $(DEP_FILES)

format: 
	clang-format -i $(C_FILES) $(H_FILES)

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/$(TARGET): $(OBJ_FILES)
	$(CC) -o $@ $^

%.o: %.c
	$(CC) $(C_FLAGS) $(DEP_FLAGS) -I$(SRC_DIR) -c -o $@ $<

-include $(DEP_FILES)

.PHONY: all directories clean
