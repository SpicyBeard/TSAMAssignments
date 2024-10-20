# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra

# Targets
TARGETS = tsamgroup42 client

# Source files
SERVER_SRC = tsamgroup42.cpp utils.cpp utils.h
CLIENT_SRC = client.cpp

# Object files
SERVER_OBJ = $(SERVER_SRC:.cpp=.o)
CLIENT_OBJ = $(CLIENT_SRC:.cpp=.o)

# Default target
all: $(TARGETS)

# Scanner target
tsamgroup42: $(SERVER_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Puzzlesolver target
client: $(CLIENT_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Clean target
clean:
	rm -f $(TARGETS) $(SERVER_OBJ) $(CLIENT_OBJ)

# Dependencies
client.o: client.cpp
utils.o: utils.cpp utils.h
tsamgroup42.o: tsamgroup42.cpp utils.h

.PHONY: all clean