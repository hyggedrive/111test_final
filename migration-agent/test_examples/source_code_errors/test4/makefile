# Compiler

ifeq ($(CXX),)
    CXX = clang++
endif


# Compiler flags
CXXFLAGS = -Wall -std=c++11

# Source files
SRC = test.cpp

# Object files
OBJ = $(SRC:.cpp=.o)

# Default rule
all:
	$(CXX) $(CXXFLAGS) -c $(SRC) -o $(OBJ)

# Clean up object and executable files
clean:
	rm -f $(OBJ)

.PHONY: all clean
