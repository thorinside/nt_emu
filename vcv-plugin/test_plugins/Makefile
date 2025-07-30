# Test Plugins Makefile for DistingNT VCV Plugin
# This builds all test plugins for development and testing

ifndef RACK_DIR
	RACK_DIR := ../Rack-SDK
endif

# Compiler settings
CXX := clang++
CXXFLAGS := -std=c++11 -fno-rtti -fno-exceptions -Os -fPIC -Wall
CXXFLAGS += -I$(RACK_DIR)/include -I$(RACK_DIR)/dep/include -I../../emulator/include
LDFLAGS := -dynamiclib -undefined dynamic_lookup

# Find all .cpp files for plugins
PLUGIN_SOURCES := $(wildcard *_plugin.cpp)
PLUGIN_DYLIBS := $(PLUGIN_SOURCES:.cpp=.dylib)

# Test executables (non-plugin tests)
TEST_SOURCES := $(filter-out %_plugin.cpp, $(wildcard test_*.cpp))
TEST_EXECUTABLES := $(TEST_SOURCES:.cpp=)

# All targets
ALL_TARGETS := $(PLUGIN_DYLIBS) $(TEST_EXECUTABLES)

# Default target
all: $(ALL_TARGETS)

# Plugin building rule
%_plugin.dylib: %_plugin.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $<
	@echo "Built plugin: $@"

# Test executable building rule  
test_%: test_%.cpp
	$(CXX) $(CXXFLAGS) -I$(RACK_DIR)/include -I$(RACK_DIR)/dep/include -I../emulator/include -o $@ $<
	@echo "Built test: $@"

# Special targets for existing dylibs (these may have been built elsewhere)
fourteen.dylib: fourteen.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $< 2>/dev/null || echo "Warning: fourteen.cpp not found, keeping existing dylib"

gain.dylib: gain.cpp  
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $< 2>/dev/null || echo "Warning: gain.cpp not found, keeping existing dylib"

scaling_demo.dylib: scaling_demo.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $< 2>/dev/null || echo "Warning: scaling_demo.cpp not found, keeping existing dylib"

simple_gain.dylib: simple_gain.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $< 2>/dev/null || echo "Warning: simple_gain.cpp not found, keeping existing dylib"

# Clean target
clean:
	rm -f *.dylib
	rm -f $(TEST_EXECUTABLES)
	rm -rf *.dSYM
	rm -f *.d *.o

# Show available targets
list:
	@echo "Available plugin targets:"
	@echo "  $(PLUGIN_DYLIBS)"
	@echo ""
	@echo "Available test executable targets:"
	@echo "  $(TEST_EXECUTABLES)"
	@echo ""
	@echo "Special targets:"
	@echo "  all     - Build all plugins and tests"
	@echo "  clean   - Remove all built files"
	@echo "  list    - Show this list"
	@echo "  plugins - Build only plugin dylibs"
	@echo "  tests   - Build only test executables"

# Convenience targets
plugins: $(PLUGIN_DYLIBS)

tests: $(TEST_EXECUTABLES)

.PHONY: all clean list plugins tests

# Print helpful information
info:
	@echo "DistingNT Test Plugins Build System"
	@echo "==================================="
	@echo "RACK_DIR: $(RACK_DIR)"
	@echo "CXX: $(CXX)"
	@echo "CXXFLAGS: $(CXXFLAGS)"
	@echo "LDFLAGS: $(LDFLAGS)"
	@echo ""
	@echo "Plugin sources found: $(PLUGIN_SOURCES)"
	@echo "Test sources found: $(TEST_SOURCES)"