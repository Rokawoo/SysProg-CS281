# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -g

# Target executable name
TARGET = stringfun-complete

# Default target
all: $(TARGET)

# Compile source to executable
$(TARGET): stringfun-complete.c
	$(CC) $(CFLAGS) -o $(TARGET) $^

# Clean up build files
clean:
	rm -f $(TARGET)

# Phony targets
.PHONY: all clean