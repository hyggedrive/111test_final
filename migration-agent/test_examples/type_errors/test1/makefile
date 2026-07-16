# Compiler

ifeq ($(CC),)
    CC = clang
endif


# Compiler flags, -fcommon下符号类型存在差异
CFLAGS = -Wall -fcommon

# Source files
SRC = test.c

# Object files
TARGET=test

# Default rule
all:
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

# Clean up object and executable files
clean:
	rm -f $(TARGET)

.PHONY: all clean
