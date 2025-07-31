# 🎯 Makefile Augmenter for DistingNT Plugins

*Because testing on actual hardware is so 2023...*

The `makefile_augmenter.py` tool is your magical bridge between the ARM Cortex-M7 world of DistingNT and the cozy comfort of your development machine. It automatically adds host platform build targets to your existing Makefiles, letting you test your plugins in the VCV Rack emulator without burning your fingers on hot hardware.

## 🤔 Why This Exists

Picture this: You've written a brilliant DistingNT plugin. It compiles perfectly for ARM. But now you want to test it without:
- Finding your DistingNT hardware under that pile of cables
- Waiting for the SD card transfer dance
- Decoding cryptic LED blink patterns when something goes wrong

This tool adds special build targets to your Makefile that compile your plugin as a native dynamic library (.dylib on macOS, .so on Linux, .dll on Windows) that the VCV Rack DistingNT emulator can load directly. Your original ARM build remains untouched—it's like having your cake and eating it too!

## 🐍 Python Setup (First Time Only)

### Check If You Have Python

```bash
python3 --version
```

If you see something like `Python 3.x.x`, you're golden! Skip to [Usage](#-usage).

### Installing Python (If Needed)

**macOS:**
```bash
# The civilized way (if you have Homebrew)
brew install python3

# Or download from python.org like a barbarian
# https://www.python.org/downloads/
```

**Linux:**
```bash
# Ubuntu/Debian
sudo apt update && sudo apt install python3

# Fedora
sudo dnf install python3

# Arch (you probably already have it)
sudo pacman -S python
```

**Windows:**
```powershell
# Download from python.org
# Or use the Microsoft Store (search for Python 3)
# Or if you're fancy: winget install Python.Python.3
```

### Make the Tool Executable (Optional but Cool)

```bash
chmod +x makefile_augmenter.py
# Now you can run it like: ./makefile_augmenter.py
```

## 🚀 Usage

### The "Just Make It Work" Command

```bash
# In your plugin directory, run:
python3 makefile_augmenter.py

# That's it. You're done. Go get coffee. ☕
```

### The "I Have Trust Issues" Command

```bash
# See what it would do without actually doing it
python3 makefile_augmenter.py --dry-run

# Like it? Remove --dry-run and run it for real
```

### The Full Monty (All Options)

```bash
python3 makefile_augmenter.py [options]

Options That Actually Matter:
  -h, --help            Show this help (you know, for help)
  -i, --input INPUT     Which Makefile to augment (default: ./Makefile)
  -o, --output OUTPUT   Where to write (default: overwrites input)
  -n, --dry-run         Preview mode - look but don't touch
  -p, --platform PLATFORM
                        Force platform: auto, macos, windows, linux, all
                        (default: auto - it's smart like that)
```

### Real-World Examples

```bash
# "I'm scared of commitment"
python3 makefile_augmenter.py --dry-run

# "My Makefile is in a weird place"
python3 makefile_augmenter.py --input plugins/dnb_seq/Makefile

# "I want a separate Makefile for VCV"
python3 makefile_augmenter.py --output Makefile.vcv

# "I only care about macOS because I'm fancy"
python3 makefile_augmenter.py --platform macos

# "I accidentally augmented twice" (it's fine, it detects this)
python3 makefile_augmenter.py
# Output: "Info: 'Makefile' appears to already have host build targets."
```

## 🎭 What It Does (The Magic Revealed)

The tool reads your Makefile like a fortune teller reads tea leaves, then adds new superpowers:

### 🧙‍♂️ Smart Detection
- **C++ Standard**: Finds if you're using C++11, C++14, C++17, or living in the future
- **Compiler Flags**: Steals the good ones (-Wall, -Os) and ignores the ARM-specific ones
- **File Extensions**: Handles .cpp, .cc, and even detects when you mix both (you rebel)
- **Include Paths**: Figures out if you use INCLUDES, INCLUDE_PATH, or something exotic

### 🎯 New Make Targets

After augmentation, your Makefile gains these shiny new commands:

```bash
# The "Build Everything" Button
make host-plugins

# The "I Only Want One" Approach
make my_awesome_plugin.dylib  # macOS
make my_awesome_plugin.so     # Linux  
make my_awesome_plugin.dll    # Windows (untested, good luck!)

# The "Spring Cleaning"
make clean-host

# The "Put It Somewhere Useful"
make install-host INSTALL_DIR=/path/to/test/plugins

# The "What Can I Do?" Helper
make help-host
```

### 🎪 Special Cases It Handles

**Multi-file plugins** (like nt_grids):
```bash
# WARNING: This Makefile appears to build multiple source files into one plugin
# The host build rules below will create separate plugins for each source file
# You may need to create a combined build rule for full functionality
```

**Faust DSP files**:
```bash
# NOTE: Original Makefile uses .dsp files
# For Faust DSP files, you'll need to compile to C++ first
# Consider using faust2vcv or similar tool
```

**Mixed .cpp and .cc files** (because consistency is overrated):
```bash
# Creates rules for both:
%.dylib: %.cpp
%.dylib: %.cc
```

## 📸 Before & After Gallery

### Example 1: Simple Plugin Makefile

**Before** (virgin Makefile):
```makefile
NT_API_PATH := ..
INCLUDE_PATH := $(NT_API_PATH)/include
inputs := $(wildcard *.cpp)
outputs := $(patsubst %.cpp,plugins/%.o,$(inputs))

all: $(outputs)

plugins/%.o: %.cpp
	arm-none-eabi-c++ -std=c++11 -mcpu=cortex-m7 -mfpu=fpv5-d16 \
	                   -mfloat-abi=hard -mthumb -fno-rtti \
	                   -fno-exceptions -Os -fPIC -Wall \
	                   -I$(INCLUDE_PATH) -c -o $@ $^
```

**After** (augmented with awesome):
```makefile
# [All your original content untouched above...]

# === VCV Emulator Test Builds (added by makefile_augmenter.py) ===
# Build plugins as host platform dynamic libraries for VCV Rack emulator testing

# Host compiler settings
HOST_CXX ?= clang++
HOST_CXXFLAGS := -std=c++11 -fPIC -Wall -Os -I$(INCLUDE_PATH)

# Detect host platform
HOST_OS := $(shell uname -s)

ifeq ($(HOST_OS),Darwin)
    HOST_SUFFIX := .dylib
    HOST_LDFLAGS := -dynamiclib -undefined dynamic_lookup
else ifeq ($(HOST_OS),Linux)
    HOST_SUFFIX := .so
    HOST_LDFLAGS := -shared
else
    # Windows/MinGW
    HOST_SUFFIX := .dll
    HOST_LDFLAGS := -shared
endif

# Use existing source file list
# Transform source files to host plugins
host_plugins := $(patsubst %.cpp,%$(HOST_SUFFIX),$(basename $(inputs)))

# Build rule for host plugins
%$(HOST_SUFFIX): %.cpp
	@echo "Building host plugin: $@"
	$(HOST_CXX) $(HOST_CXXFLAGS) $(HOST_LDFLAGS) -o $@ $<

# ... convenience targets follow ...
```

### Example 2: Complex Multi-File Plugin

**Input**: A Makefile that builds multiple .cc files into one plugin

**Output includes**:
```makefile
# NOTE: Original Makefile uses .cc files

# WARNING: This Makefile appears to build multiple source files into one plugin
# The host build rules below will create separate plugins for each source file
# You may need to create a combined build rule for full functionality

# Build rule for host plugins
%$(HOST_SUFFIX): %.cc
	@echo "Building host plugin: $@"
	$(HOST_CXX) $(HOST_CXXFLAGS) $(HOST_LDFLAGS) -o $@ $<
```

## 🛡️ Safety Features

Because we care about your Makefiles:

- **🔒 Backup Creation**: Original saved to `Makefile.bak` (first time only)
- **🔁 Idempotent**: Run it twice? No problem. It knows.
- **🎯 Surgical Precision**: Your original content remains untouched
- **📍 Clear Markers**: Added content wrapped in obvious comments

## 🎨 Pro Customization

For the power users who want to tweak everything:

```bash
# "I prefer g++ because I'm old school"
HOST_CXX=g++ make host-plugins

# "I want ALL the optimizations"
HOST_CXXFLAGS="-std=c++17 -fPIC -Wall -O3 -march=native" make host-plugins

# "My plugins belong in a specific place"
make install-host INSTALL_DIR=~/.Rack2/plugins-dev/test

# "I'm building on an M1 Mac and things are weird"
HOST_CXX="clang++ -arch x86_64" make host-plugins
```

## 🔥 Troubleshooting (When Things Go Wrong)

### "It says my Makefile is already augmented"
**Translation**: You already ran this tool. You're good.
**Action**: Make some tea. You're done.

### "My plugin won't build for host"
**Common Causes**:
- Using ARM-specific intrinsics (replace with portable code)
- Missing compiler (`brew install llvm` or `apt install build-essential`)
- Include path confusion (check your NT_API_PATH)

**Debug Commands**:
```bash
# See what's actually happening
make host-plugins VERBOSE=1

# Check your compiler
which clang++ || which g++

# Verify includes
echo $NT_API_PATH
```

### "VCV can't load my plugin"
**The Usual Suspects**:
- Built for wrong architecture (M1 Mac vs Intel?)
- Missing -fPIC flag (but we add it automatically)
- Plugin doesn't implement required `_NT_*` functions

**Quick Test**:
```bash
# Check if it's a valid dynamic library
file my_plugin.dylib  # macOS
ldd my_plugin.so      # Linux
```

### "It made rules for each .cc file but I need them combined"
**For multi-file plugins**, add this to your Makefile manually:
```makefile
# Combined build rule for multi-file plugin
my_plugin$(HOST_SUFFIX): file1.cc file2.cc file3.cc
	$(HOST_CXX) $(HOST_CXXFLAGS) $(HOST_LDFLAGS) -o $@ $^
```

## 🚁 Advanced Scenarios

### Scenario: "I have Faust DSP files"
The tool will detect .dsp files and create placeholder rules. You'll need to:
1. First convert .dsp → .cpp using faust2distingnt or similar
2. Then build the resulting .cpp files for host

### Scenario: "My Makefile is generated by CMake"
1. Augment the generated Makefile
2. Add to your CMakeLists.txt to preserve changes:
   ```cmake
   # At the end of CMakeLists.txt
   add_custom_target(augment
     COMMAND python3 path/to/makefile_augmenter.py
     COMMENT "Adding host build targets"
   )
   ```

### Scenario: "I want this in my GitHub Actions"
```yaml
- name: Augment Makefile for host builds
  run: |
    python3 tools/makefile_augmenter.py
    make host-plugins
    make check-host  # if you have tests
```

## 🎬 The End

Remember: This tool turns your ARM-only Makefile into a dual-platform powerhouse. Your original ARM builds remain untouched while gaining the ability to build native plugins for testing.

Happy plugin development! May your DSP be glitch-free and your modulations smooth. 🎵

---

**Found a bug?** The tool is probably fine. Your Makefile might be haunted.  
**Need help?** Read this document again, but with feeling.  
**Still stuck?** Check if your computer is plugged in.

*Created with love for the DistingNT community by someone who got tired of copying plugins to SD cards.*