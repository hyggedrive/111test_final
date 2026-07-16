# Compiler

ifeq ($(CC),)
    CC = clang
endif


# Compiler flags
CFLAGS = -Wall 

# Output executable name
TARGET = test

# Source files
SRC = test.c

# Object files
OBJ = $(SRC:.c=.o)

# # Default rule
all: $(TARGET)
# all:
# 	$(CC) $(CFLAGS) -c $(SRC) -o $(OBJ)

# Rule to create the executable
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

# Rule to compile .cpp files into .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up object and executable files
clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
