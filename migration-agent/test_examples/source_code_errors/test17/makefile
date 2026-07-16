# Compiler

ifeq ($(CC),)
    CC = clang
endif


# Compiler flags
CCFLAGS = -Wall

# Source files
SRC = test.c

# Object files
OBJ = $(SRC:.c=.o)

# Default rule
all:
	$(CC) $(CCFLAGS) -c $(SRC) -o $(OBJ)

# Clean up object and executable files
clean:
	rm -f $(OBJ)

.PHONY: all clean
