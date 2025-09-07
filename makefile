CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -Iinclude
SRC := src/main.cpp \
	src/loadConfig.cpp \
	src/return404.cpp \
	src/returnDirListing.cpp \
	src/returnErrorPage.cpp \
	src/logRequest.cpp \
	src/headerManager.cpp \
	src/evaluateTrust.cpp \
	src/perMinute404.cpp
OBJ := $(SRC:.cpp=.o)
BIN := faucet

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $@

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(BIN)

.PHONY: all clean