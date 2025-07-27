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

# Fix for rsync issue - override the dist target
dist: all
	rm -rf dist
	mkdir -p dist/$(SLUG)
	cp plugin.dylib dist/$(SLUG)/
	strip -S dist/$(SLUG)/plugin.dylib
ifdef ARCH_MAC
	install_name_tool -change libRack.dylib /tmp/Rack2/libRack.dylib dist/$(SLUG)/plugin.dylib
	otool -L dist/$(SLUG)/plugin.dylib
	codesign -f -s - dist/$(SLUG)/plugin.dylib
endif
	cp -R $(DISTRIBUTABLES) dist/$(SLUG)/
	cd dist && tar --no-xattrs -c $(SLUG) | zstd -19 -o $(SLUG)-$(VERSION)-$(ARCH).vcvplugin