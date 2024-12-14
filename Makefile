DEBUG_DIR := debug-demo
RELEASE_DIR := release-demo
BENCHMARK_DIR := benchmark

DEBUG_BUILD_TYPE := Debug
RELEASE_BUILD_TYPE := RelWithDebInfo

.PHONY: python-module
python-module:
	@pip install .

.PHONY: debug-demo
debug-demo:
	@mkdir -p $(DEBUG_DIR)
	@cd $(DEBUG_DIR) && \
	cmake -GNinja -DCMAKE_BUILD_TYPE=$(DEBUG_BUILD_TYPE) -DBUILD_DEMO=true .. && \
	cmake --build .

.PHONY: release-demo
release-demo:
	@mkdir -p $(RELEASE_DIR)
	@cd $(RELEASE_DIR) && \
	cmake -GNinja -DCMAKE_BUILD_TYPE=$(RELEASE_BUILD_TYPE) -DBUILD_DEMO=true .. && \
	cmake --build .

.PHONY: benchmark
benchmark:
	@mkdir -p $(BENCHMARK_DIR)
	@cd $(BENCHMARK_DIR) && \
	cmake -GNinja -DCMAKE_BUILD_TYPE=$(RELEASE_BUILD_TYPE) -DBUILD_BENCHMARK=true .. && \
	cmake --build .

.PHONY: clean
clean:
	@rm -rf build $(DEBUG_DIR) $(RELEASE_DIR) $(BENCHMARK_DIR)
