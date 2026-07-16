# Compiler

ifeq ($(CXX),)
    CXX = clang++
endif


# Compiler flags
CXXFLAGS = -Wall -std=c++14

# Source files
SRC = func.cpp main.cpp

# Object files
OBJ = $(SRC:.cpp=.o)

# Default rule
all:
	$(CXX) $(CXXFLAGS) $(SRC) 

# Clean up object and executable files
clean:
	rm -f $(OBJ) a.out *.log

.PHONY: all clean
