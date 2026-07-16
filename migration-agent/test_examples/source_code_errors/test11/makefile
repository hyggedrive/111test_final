# Compiler

ifeq ($(CXX),)
    CXX = clang++
endif


# Compiler flags
CXXFLAGS = -Wall -std=c++11

# Output executable name
TARGET = test

# Source files
SRC = test.cpp

# Object files
OBJ = $(SRC:.cpp=.o)

# Default rule
# all: $(TARGET)
all:
	$(CC) $(CFLAGS) -c $(SRC) -o $(OBJ)

# # Rule to create the executable
# $(TARGET): $(OBJ)
# 	$(CXX) $(OBJ) -o $(TARGET)

# # Rule to compile .cpp files into .o files
# %.o: %.cpp
# 	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up object and executable files
clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
