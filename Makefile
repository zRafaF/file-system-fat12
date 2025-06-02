# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -Iinclude

# Directories
SRC_DIR = src
BUILD_DIR = build

# Source and object files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))


# Executable
TARGET = file_system

# Ensure build directory exists
ifeq ($(OS),Windows_NT)
    dir_guard = @if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
else
    dir_guard = @mkdir -p $(BUILD_DIR)
endif

# Phony targets
.PHONY: all clean run mor

# Build rule
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(clean)
	$(dir_guard)
	$(CC) $(CFLAGS) -c $< -o $@

# Main target with clean and all as dependencies
$(TARGET): clean all

all:
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)
	@echo Build complete for $(TARGET)


run: $(TARGET)
ifeq ($(OS),Windows_NT)
	@if exist $(TARGET).exe .\\$(TARGET).exe
else
	./$(TARGET)
endif

clean:
	@echo Cleaning $(TARGET)
ifeq ($(OS),Windows_NT)
	@if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR) 2>nul
	@if exist $(TARGET).exe del /f $(TARGET).exe 2>nul
else
	@if [ -d "$(BUILD_DIR)" ]; then rm -rf $(BUILD_DIR); fi
	@if [ -e "$(TARGET)" ]; then rm -f $(TARGET); fi
endif