# Top-level convenience Makefile to drive the CMake build in ./build

BUILD_DIR ?= build
CMAKE ?= cmake
CMAKE_FLAGS ?= -DCMAKE_BUILD_TYPE=Release

.PHONY: all clean configure rebuild

all: $(BUILD_DIR)/Makefile
	$(MAKE) -C $(BUILD_DIR)

$(BUILD_DIR)/Makefile:
	@mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && $(CMAKE) $(CMAKE_FLAGS) ..

configure: $(BUILD_DIR)/Makefile

clean:
	@if [ -d $(BUILD_DIR) ]; then \
		$(MAKE) -C $(BUILD_DIR) clean; \
		rm -f $(BUILD_DIR)/CMakeCache.txt; \
	fi

rebuild: clean all
