# Compiler

ifeq ($(CC),)
    CC = clang
endif


# Compiler flags, -nostdinc -iwithprefix include差异
CFLAGS = -nostdinc  -iwithprefix  include -Wall

# Source files
SRC = test.c

# Object files
OBJ = $(SRC:.c=.o)

# Default rule
all:
	$(CC) $(CFLAGS) -c $(SRC) -o $(OBJ)

# Clean up object and executable files
clean:
	rm -f $(OBJ) *.log

.PHONY: all clean
