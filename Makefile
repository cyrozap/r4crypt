CXX := g++
CXXFLAGS := -Wall -g

BUILD_DIR := build
SRC_DIR := source

CPP_FILES := $(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES := $(addprefix $(BUILD_DIR)/,$(notdir $(CPP_FILES:.cpp=.o)))

PREFIX   := /usr/local


.PHONY: all checkdirs clean install

all: checkdirs r4crypt

checkdirs: $(BUILD_DIR)

$(BUILD_DIR):
	@mkdir -p $@

r4crypt: $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

install:
	install -m 0755 r4crypt $(PREFIX)/bin

clean:
	@rm -rf r4crypt $(BUILD_DIR)
