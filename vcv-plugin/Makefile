# Standard VCV Rack plugin Makefile

# Plugin details
SLUG = DistingNT
VERSION = 2.0.0

# Rack SDK
RACK_DIR ?= ./Rack-SDK

# Add official Disting NT API include path (from submodule)
FLAGS += -I../external/distingNT_API/include

# Plugin source files
SOURCES += $(wildcard src/*.cpp)
SOURCES += $(wildcard src/*/*.cpp)

# Add VCV-specific fonts implementation for shared font system
SOURCES += src/fonts_vcv.cpp

# Add ApiShim for drawing API support (commented out due to memory corruption)
# SOURCES += ../emulator/src/core/api_shim.cpp

# Add MidiHandler source (commented out due to compilation issues)
# SOURCES += ../emulator/src/core/midi_handler.cpp

# Distributed plugins include the license and resources
DISTRIBUTABLES += LICENSE.txt
DISTRIBUTABLES += res

# Include the VCV Rack plugin Makefile framework
include $(RACK_DIR)/plugin.mk

# Use default dist target from plugin.mk

# Test plugin targets
TEST_PLUGINS = test_customui_plugin.dylib drawtest_plugin.dylib fonttest_plugin.dylib

test_customui_plugin.dylib: test_customui_plugin.cpp
	$(CXX) $(CXXFLAGS) $(FLAGS) -shared -o $@ $< -I../external/distingNT_API/include $(LDFLAGS)

drawtest_plugin.dylib: drawtest_plugin.cpp
	$(CXX) $(CXXFLAGS) $(FLAGS) -shared -o $@ $< -I../external/distingNT_API/include $(LDFLAGS)

fonttest_plugin.dylib: fonttest_plugin.cpp
	$(CXX) $(CXXFLAGS) $(FLAGS) -shared -o $@ $< -I../external/distingNT_API/include $(LDFLAGS)

.PHONY: test-plugins
test-plugins: $(TEST_PLUGINS)

.PHONY: clean-test-plugins
clean-test-plugins:
	rm -f $(TEST_PLUGINS)

# JSON Bridge Unit Tests
TEST_SOURCES = tests/test_json_bridge_standalone.cpp
TEST_FLAGS = -I../external/distingNT_API/include -std=c++11 -stdlib=libc++
TEST_BINARY = tests/test_json_bridge_standalone

$(TEST_BINARY): $(TEST_SOURCES)
	$(CXX) $(TEST_FLAGS) -o $@ $(TEST_SOURCES)

.PHONY: test
test: $(TEST_BINARY)
	@echo "Running JSON Bridge Unit Tests..."
	@./$(TEST_BINARY)

.PHONY: clean-tests
clean-tests:
	rm -f $(TEST_BINARY)

# Add tests to main clean target
clean: clean-tests

# Example plugins from official API - compile as macOS dylibs for VCV emulator testing
EXAMPLE_SRC_DIR = ../external/distingNT_API/examples
EXAMPLE_OUT_DIR = plugins
EXAMPLE_SOURCES = $(wildcard $(EXAMPLE_SRC_DIR)/*.cpp)
EXAMPLE_PLUGINS = $(patsubst $(EXAMPLE_SRC_DIR)/%.cpp,$(EXAMPLE_OUT_DIR)/%.dylib,$(EXAMPLE_SOURCES))

# Compiler flags for example plugins (standalone dylibs, not VCV modules)
EXAMPLE_CXX = c++
EXAMPLE_CXXFLAGS = -std=c++11 -stdlib=libc++ -fPIC -O3 -I../external/distingNT_API/include
EXAMPLE_LDFLAGS = -dynamiclib -undefined dynamic_lookup

$(EXAMPLE_OUT_DIR)/%.dylib: $(EXAMPLE_SRC_DIR)/%.cpp
	@mkdir -p $(EXAMPLE_OUT_DIR)
	$(EXAMPLE_CXX) $(EXAMPLE_CXXFLAGS) $(EXAMPLE_LDFLAGS) -o $@ $<

.PHONY: example-plugins
example-plugins: $(EXAMPLE_PLUGINS)
	@echo "Built $(words $(EXAMPLE_PLUGINS)) example plugins to $(EXAMPLE_OUT_DIR)/"

.PHONY: clean-example-plugins
clean-example-plugins:
	rm -rf $(EXAMPLE_OUT_DIR)

.PHONY: list-example-plugins
list-example-plugins:
	@echo "Available example plugins:"
	@for src in $(EXAMPLE_SOURCES); do echo "  - $$(basename $$src .cpp)"; done

