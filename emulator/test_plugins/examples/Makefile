# Makefile for DistingNT API example plugins
CXX = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -fPIC -I../../include
LDFLAGS = -shared

# List of all plugins to build
PLUGINS = gain.dylib gainCustomUI.dylib gainMultichannel.dylib flexSeqSwitch.dylib \
          fourteen.dylib midiLFO.dylib modJulia.dylib monosynth.dylib \
          multiple.dylib oxiremote.dylib

# Default target builds all plugins
all: $(PLUGINS)

# Pattern rule for building plugins (runtime symbol resolution)
%.dylib: %.cpp
	@echo "Building $@..."
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $< -undefined dynamic_lookup

# Target for standalone testing with stubs (when emulator not available)
%-standalone.dylib: %.cpp nt_api_stubs.cpp
	@echo "Building $@ (standalone with stubs)..."
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $< nt_api_stubs.cpp

# Legacy rule kept for compatibility  
gainCustomUI-nostubs.dylib: gainCustomUI.cpp
	@echo "Building $@ (without API stubs for VCV plugin)..."
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $< -undefined dynamic_lookup

# Clean target
clean:
	rm -f *.dylib *.o

# Test target - compile but don't link (for syntax checking)
test-compile: $(PLUGINS:.dylib=.o)

%.o: %.cpp
	@echo "Compiling $@ (syntax check)..."
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Help target
help:
	@echo "Available targets:"
	@echo "  all           - Build all plugins"
	@echo "  clean         - Remove built files"  
	@echo "  test-compile  - Compile all plugins (syntax check only)"
	@echo "  help          - Show this help"
	@echo ""
	@echo "Individual plugins can be built with: make <plugin>.dylib"
	@echo "Available plugins: $(PLUGINS)"

.PHONY: all clean test-compile help