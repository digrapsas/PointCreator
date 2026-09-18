# "all" is the name of the default target, running "make" without params would use it
all: main

# compiler and flags
CXX      = g++
CXXFLAGS = -pthread -std=c++17 -pedantic -Wall -Wextra -Wshadow -Wconversion -Wunreachable-code
OPTFLAGS = -O3 -DNDEBUG -march=native
COMPILE  = $(CXX) $(CXXFLAGS)

release: CXXFLAGS += $(OPTFLAGS)
release: main

debug: CXXFLAGS += -g -DDEBUG
debug: main

# directories
BUILD = ./build
SRC   = ./src
RES   = ./res

# files
OBJ = $(BUILD)/main.o
CPP = $(SRC)/*.cpp
EXE = $(BUILD)/pc.out

# rules
# -c flag for no linking
main: $(OBJ)
	$(COMPILE)     -o  $(EXE) $(OBJ)

./build/main.o: main.cpp build
	$(COMPILE)  -c -o  $(BUILD)/main.o main.cpp 

# Make the build directory if it doesn't exist
build:
	mkdir -p $(BUILD) $(RES)

clean: 
	rm -rf $(BUILD)

cleanres: 
	rm -rf $(RES)

cleanall: 
	rm -rf $(RES) $(BUILD)

# These rules do not correspond to a specific file
.PHONY: build clean cleanres cleanall run