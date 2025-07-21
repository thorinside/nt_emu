# Standard VCV Rack plugin Makefile

# Plugin details
SLUG = DistingNT
VERSION = 2.0.0

# Rack SDK
RACK_DIR ?= ./Rack-SDK

# Plugin source files
SOURCES += $(wildcard src/*.cpp)
SOURCES += $(wildcard src/*/*.cpp)

# Distributed plugins include the license and resources
DISTRIBUTABLES += LICENSE.txt
DISTRIBUTABLES += res

# Include the VCV Rack plugin Makefile framework
include $(RACK_DIR)/plugin.mk