BUILD_DIR := build
BUILD_DIR_FILE := $(BUILD_DIR)/.empty

ifdef DEBUG 
BUILD_TYPE := debug
else
BUILD_TYPE := minsizerel
endif

default: all

$(BUILD_DIR_FILE):
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) ..
	@touch $(BUILD_DIR_FILE)

configure: $(BUILD_DIR_FILE)
	
all: $(BUILD_DIR_FILE)
	@make --silent -C $(BUILD_DIR)

run_unit_test: all
	@$(BUILD_DIR)/JsonDb_unit_test

clean:
	@rm -rf $(BUILD_DIR)

