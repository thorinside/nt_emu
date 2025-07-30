# Standard VCV Rack plugin Makefile

# Plugin details
SLUG = DistingNT
VERSION = 2.0.0

# Rack SDK
RACK_DIR ?= ./Rack-SDK

# Add emulator include path
FLAGS += -I../emulator/include

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
	$(CXX) $(CXXFLAGS) $(FLAGS) -shared -o $@ $< -I../emulator/include $(LDFLAGS)

drawtest_plugin.dylib: drawtest_plugin.cpp
	$(CXX) $(CXXFLAGS) $(FLAGS) -shared -o $@ $< -I../emulator/include $(LDFLAGS)

fonttest_plugin.dylib: fonttest_plugin.cpp
	$(CXX) $(CXXFLAGS) $(FLAGS) -shared -o $@ $< -I../emulator/include $(LDFLAGS)

.PHONY: test-plugins
test-plugins: $(TEST_PLUGINS)

.PHONY: clean-test-plugins
clean-test-plugins:
	rm -f $(TEST_PLUGINS)

# JSON Bridge Unit Tests
TEST_SOURCES = tests/test_json_bridge_standalone.cpp
TEST_FLAGS = -I../emulator/include -std=c++11 -stdlib=libc++
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