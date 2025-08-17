APP := screensaver
BUILD_DIR := build
SRC := src/main.cpp
CXX := g++
CXXFLAGS := -O2 -Wall -Wextra -std=c++17 $(shell sdl2-config --cflags)
LDFLAGS := $(shell sdl2-config --libs)

$(BUILD_DIR)/$(APP): $(SRC)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $(BUILD_DIR)/$(APP) $(SRC) $(LDFLAGS)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
