RELEASE_PYTHON_MODULE_DIR := python-module-release
DEBUG_PYTHON_MODULE_DIR := python-module-debug
DEBUG_DIR := debug-demo
RELEASE_DIR := release-demo
BENCHMARK_DIR := benchmark

DEBUG_BUILD_TYPE := Debug
RELEASE_BUILD_TYPE := RelWithDebInfo

# install build dependencies if this is a fresh build, Python won't
# install build dependencies when --no-build-isolation is passed
# build with no isolation so that builds can be cached and/or incremental
.PHONY: python-module-release
python-module-release:
	@test -d $(RELEASE_PYTHON_MODULE_DIR) || pip install scikit-build-core autopxd2 cython
	@pip install --no-build-isolation --config-settings=editable.rebuild=true -Cbuild-dir=$(RELEASE_PYTHON_MODULE_DIR) -v .

.PHONY: python-module-debug
python-module-debug:
	@test -d $(DEBUG_PYTHON_MODULE_DIR) || pip install scikit-build-core autopxd2 cython
	@pip install --no-build-isolation --config-settings=editable.rebuild=true --config-settings=cmake.build-type="Debug" -Cbuild-dir=$(DEBUG_PYTHON_MODULE_DIR) -v .	

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
	@rm -rf build $(RELEASE_PYTHON_MODULE_DIR) $(DEBUG_PYTHON_MODULE_DIR) $(DEBUG_DIR) $(RELEASE_DIR) $(BENCHMARK_DIR)
