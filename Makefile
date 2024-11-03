# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -g -fsanitize=address

# Directories
SRC_DIR = ./src
BUILD_DIR = ./build
BIN_DIR = ./bin

# Files
MAIN_SRC := $(SRC_DIR)/main.c
MAIN_OBJ := $(BUILD_DIR)/main.o
MAIN_BIN := $(BIN_DIR)/main

# Find all .c files not ending with _test.c in the src directory
SOURCES = $(filter-out %_test.c, $(filter-out $(SRC_DIR)/main.c, $(wildcard $(SRC_DIR)/*.c)))

# Generate object file names for non-test files
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Find all test files in the src directory
TEST_SOURCES = $(wildcard $(SRC_DIR)/*_test.c)

# Generate names for test executables
TEST_EXECUTABLES = $(TEST_SOURCES:$(SRC_DIR)/%_test.c=$(BUILD_DIR)/%_test)

# Default target builds all objects and test executables
all: $(BUILD_DIR) $(BIN_DIR) $(OBJECTS) $(TEST_EXECUTABLES) $(MAIN_BIN)

# Build main executable
$(MAIN_BIN): $(BUILD_DIR)/main.o $(OBJECTS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

# Rule to create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Rule to build test executables
$(BUILD_DIR)/%_test: $(SRC_DIR)/%_test.c $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

# Rule to build object files from non-test .c files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR):
	mkdir -p $@

# Clean rule
clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(BIN_DIR)

# Test command to run all test executables
test: clean all
	@for test in $(TEST_EXECUTABLES); do \
		echo "Running $$test..."; \
		$$test; \
		if [ $$? -ne 0 ]; then \
			echo "$$test failed"; \
			exit 1; \
		fi; \
	done
	@echo "All tests passed successfully!"

debug: test
	@./bin/main

# Phony targets
.PHONY: all clean test debug
