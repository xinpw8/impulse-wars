DEBUG_DIR := debug
RELEASE_DIR := release

DEBUG_BUILD_TYPE := Debug
RELEASE_BUILD_TYPE := RelWithDebInfo

.PHONY: all
all: debug release

.PHONY: debug
debug:
	@cd $(DEBUG_DIR) && cmake --build .

.PHONY: debug-full
debug-full:
	@mkdir -p $(DEBUG_DIR)
	@cd $(DEBUG_DIR) && \
	cmake -GNinja -DCMAKE_BUILD_TYPE=$(DEBUG_BUILD_TYPE) .. && \
	cmake --build .

.PHONY: release
release:
	@cd $(RELEASE_DIR) && cmake --build .

.PHONY: release-full
release-full:
	@mkdir -p $(RELEASE_DIR)
	@cd $(RELEASE_DIR) && \
	cmake -GNinja -DCMAKE_BUILD_TYPE=$(RELEASE_BUILD_TYPE) .. && \
	cmake --build .

.PHONY: clean
clean:
	@rm -rf build $(DEBUG_DIR) $(RELEASE_DIR)
