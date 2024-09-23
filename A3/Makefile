# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra

# Targets
TARGETS = scanner puzzlesolver

# Source files
SCANNER_SRC = scanner.cpp
PUZZLESOLVER_SRC = puzzlesolver.cpp common.cpp PortSolvers/secret_port.cpp PortSolvers/checksum_port.cpp PortSolvers/evil_port.cpp PortSolvers/oracle_port.cpp

# Object files
SCANNER_OBJ = $(SCANNER_SRC:.cpp=.o)
PUZZLESOLVER_OBJ = $(PUZZLESOLVER_SRC:.cpp=.o)

# Default target
all: $(TARGETS)

# Scanner target
scanner: $(SCANNER_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Puzzlesolver target
puzzlesolver: $(PUZZLESOLVER_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Clean target
clean:
	rm -f $(TARGETS) $(SCANNER_OBJ) $(PUZZLESOLVER_OBJ)

# Dependencies
scanner.o: scanner.cpp
common.o: common.cpp common.h
puzzlesolver.o: puzzlesolver.cpp common.h PortSolvers/secret_port.h PortSolvers/checksum_port.h PortSolvers/evil_port.h PortSolvers/oracle_port.h
PortSolvers/secret_port.o: PortSolvers/secret_port.cpp PortSolvers/secret_port.h
PortSolvers/checksum_port.o: PortSolvers/checksum_port.cpp PortSolvers/checksum_port.h
PortSolvers/evil_port.o: PortSolvers/evil_port.cpp PortSolvers/evil_port.h
PortSolvers/oracle_port.o: PortSolvers/oracle_port.cpp PortSolvers/oracle_port.h

.PHONY: all clean