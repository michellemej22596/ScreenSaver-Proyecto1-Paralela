CXX = g++
CXXFLAGS = -O3 -Wall -std=c++17
LDFLAGS = -lSDL2

SRC_DIR = src
BIN_DIR = bin

SEQ_SRC = $(SRC_DIR)/screensaver_seq.cpp
PAR_SRC = $(SRC_DIR)/screensaver_par.cpp

SEQ_BIN = $(BIN_DIR)/screensaver_seq
PAR_BIN = $(BIN_DIR)/screensaver_par

all: $(SEQ_BIN) $(PAR_BIN)

$(SEQ_BIN): $(SEQ_SRC) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

$(PAR_BIN): $(PAR_SRC) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -fopenmp $< -o $@ $(LDFLAGS)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(BIN_DIR)
