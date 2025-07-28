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