DEBUG_DIR := debug
RELEASE_DIR := release

DEBUG_BUILD_TYPE := Debug
RELEASE_BUILD_TYPE := RelWithDebInfo

.PHONY: all
all: debug release

.PHONY: debug
debug:
	@cd $(DEBUG_DIR) && cmake --build . -j 24

.PHONY: debug-full
debug-full:
	@mkdir -p $(DEBUG_DIR)
	@cd $(DEBUG_DIR) && cmake -DCMAKE_BUILD_TYPE=$(DEBUG_BUILD_TYPE) .. && cmake --build . -j 24

.PHONY: release
release:
	@cd $(RELEASE_DIR) && cmake --build . -j 24

.PHONY: release-full
release-full:
	@mkdir -p $(RELEASE_DIR)
	@cd $(RELEASE_DIR) && cmake -DCMAKE_BUILD_TYPE=$(RELEASE_BUILD_TYPE) .. && cmake --build . -j 24

.PHONY: clean
clean:
	@rm -rf $(DEBUG_DIR) $(RELEASE_DIR)
