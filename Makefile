BUILD_DIR := build
BUILD_DIR_FILE := $(BUILD_DIR)/.empty

default: all

$(BUILD_DIR_FILE):
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=debug ..
	touch $(BUILD_DIR_FILE)

configure: $(BUILD_DIR_FILE)
	
all: $(BUILD_DIR_FILE)
	make -C $(BUILD_DIR)

run_unit_test: all
	$(BUILD_DIR)/JsonDb_unit_test

clean:
	rm -rf $(BUILD_DIR)

