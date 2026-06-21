
ARGS ?= ls -la
BUILD_DIR = build

all: build

$(BUILD_DIR)/CMakeCache.txt:
	@mkdir -p $(BUILD_DIR)
	@cmake -S . -B $(BUILD_DIR)

build: $(BUILD_DIR)/CMakeCache.txt
	@cmake --build $(BUILD_DIR)

run: build
	@./$(BUILD_DIR)/pmon $(ARGS)

test: build
	@cd $(BUILD_DIR) && ctest --output-on-failure

clean:
	@rm -rf $(BUILD_DIR)/*

.PHONY: all build run test clean
