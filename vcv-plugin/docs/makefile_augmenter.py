#!/usr/bin/env python3
"""
Makefile Augmenter for DistingNT Plugins

This tool adds host platform build targets to existing ARM Cortex-M7 Makefiles,
allowing plugins to be built as dynamic libraries for testing in the VCV Rack emulator.

Usage:
    python makefile_augmenter.py [--input Makefile] [--output Makefile.augmented] [--platform auto|macos|windows|linux|all]
"""

import argparse
import os
import re
import sys
from typing import List, Dict, Tuple, Optional


class MakefileAugmenter:
    def __init__(self):
        self.lines = []
        self.variables = {}
        self.targets = {}
        self.augmented = False
        self.compiler_flags = []
        self.cpp_standard = 'c++11'
        self.source_extension = '.cpp'
        self.custom_build_commands = []
        
    def parse_makefile(self, content: str) -> None:
        """Parse the Makefile content and extract key information."""
        self.lines = content.splitlines()
        
        # Extract variables and analyze build rules
        for i, line in enumerate(self.lines):
            # Match variable assignments (VAR := value or VAR = value)
            var_match = re.match(r'^(\w+)\s*[:?]?=\s*(.*)$', line)
            if var_match:
                var_name = var_match.group(1)
                var_value = var_match.group(2)
                self.variables[var_name] = (var_value, i)
            
            # Extract compiler flags from ARM build rules
            if 'arm-none-eabi-c++' in line or 'arm-none-eabi-g++' in line:
                self._extract_compiler_info(line)
            # Also check for flags in variable definitions like CXXFLAGS
            elif 'CXXFLAGS' in line and '-std=' in line:
                self._extract_compiler_info(line)
            
            # Check for custom build tools (like faust2distingnt)
            if line.strip().startswith('\t') and not line.strip().startswith('\t@'):
                if 'faust2distingnt' in line or 'custom' in line.lower():
                    self.custom_build_commands.append(line.strip())
            
            # Check if already augmented
            if 'VCV Emulator Test Builds' in line:
                self.augmented = True
                
    def _extract_compiler_info(self, line: str) -> None:
        """Extract compiler flags and standards from ARM build commands."""
        # Extract C++ standard
        std_match = re.search(r'-std=(gnu\+\+|c\+\+)(\d+)', line)
        if std_match:
            # Get just the standard part without -std=
            self.cpp_standard = std_match.group(1) + std_match.group(2)
            self.cpp_standard = self.cpp_standard.replace('gnu++', 'c++')
        
        # Extract useful flags (exclude ARM-specific ones)
        flag_patterns = [
            r'-Wall', r'-Wno-\w+', r'-O[s0-3]', r'-g', r'-MMD', r'-MP',
            r'-ffunction-sections', r'-fdata-sections'
        ]
        for pattern in flag_patterns:
            flags = re.findall(pattern, line)
            self.compiler_flags.extend(flags)
                
    def find_source_patterns(self) -> Tuple[str, str]:
        """Find patterns for source files in the Makefile and detect file type."""
        patterns = []
        extension = '.cpp'
        
        # Look for wildcard patterns and detect extension
        for var_name, (value, _) in self.variables.items():
            if 'wildcard' in value:
                patterns.append(value)
                # Extract extension from wildcard - handle both *.cpp and *cpp patterns
                ext_match = re.search(r'\*\.?(\w+)', value)
                if ext_match:
                    extension = '.' + ext_match.group(1)
                    self.source_extension = extension
            elif var_name.lower() in ['inputs', 'sources', 'srcs', 'src_files', 'src', 'source']:
                patterns.append(value)
                # Also check for explicit file lists with extensions
                # Check for mixed first!
                if '.cc' in value and '.cpp' in value:
                    # Mixed extensions - this is complex
                    extension = 'mixed'
                    self.source_extension = 'mixed'
                elif '.cc' in value:
                    extension = '.cc'
                    self.source_extension = extension
                elif '.cpp' in value:
                    extension = '.cpp'
                    self.source_extension = extension
                
        return patterns, extension
    
    def generate_host_targets(self, platform: str = 'auto') -> str:
        """Generate host platform build targets."""
        targets = []
        
        # Find source patterns and update extension
        patterns, detected_ext = self.find_source_patterns()
        
        # Header comment
        targets.append("\n# === VCV Emulator Test Builds (added by makefile_augmenter.py) ===")
        targets.append("# Build plugins as host platform dynamic libraries for VCV Rack emulator testing")
        
        # Note about non-C++ sources and multi-file projects
        if self.source_extension != '.cpp':
            targets.append(f"# NOTE: Original Makefile uses {self.source_extension} files")
            if self.source_extension == '.dsp':
                targets.append("# For Faust DSP files, you'll need to compile to C++ first")
                targets.append("# Consider using faust2vcv or similar tool")
            targets.append("")
            
        # Check if this appears to be a multi-file project
        if 'SOURCES' in self.variables or 'OBJECTS' in self.variables:
            sources_value = self.variables.get('SOURCES', ('', 0))[0]
            if sources_value and len(sources_value.split()) > 3:
                targets.append("# WARNING: This Makefile appears to build multiple source files into one plugin")
                targets.append("# The host build rules below will create separate plugins for each source file")
                targets.append("# You may need to create a combined build rule for full functionality")
                targets.append("")
        
        # Compiler and base flags
        targets.append("# Host compiler settings")
        targets.append("HOST_CXX ?= clang++")
        
        # Use detected C++ standard and flags
        extra_flags = ' '.join(set(self.compiler_flags))  # Remove duplicates
        
        # Determine include path variable
        include_var = "-I$(INCLUDE_PATH)"
        if 'INCLUDES' in self.variables:
            # Check if INCLUDES already has -I prefix
            includes_val = self.variables['INCLUDES'][0]
            if '-I' in includes_val:
                include_var = "$(INCLUDES)"
            else:
                include_var = "-I$(INCLUDES)"
        elif 'INCLUDE_PATH' not in self.variables and 'NT_API_PATH' in self.variables:
            # If INCLUDE_PATH not defined but NT_API_PATH is, define it
            targets.append("INCLUDE_PATH ?= $(NT_API_PATH)/include")
            
        targets.append(f"HOST_CXXFLAGS := -std={self.cpp_standard} -fPIC -Wall {extra_flags} {include_var}".rstrip())
        targets.append("")
        
        # Platform detection
        if platform == 'auto' or platform == 'all':
            targets.append("# Detect host platform")
            targets.append("HOST_OS := $(shell uname -s)")
            targets.append("")
            targets.append("ifeq ($(HOST_OS),Darwin)")
            targets.append("    HOST_SUFFIX := .dylib")
            targets.append("    HOST_LDFLAGS := -dynamiclib -undefined dynamic_lookup")
            targets.append("else ifeq ($(HOST_OS),Linux)")
            targets.append("    HOST_SUFFIX := .so")
            targets.append("    HOST_LDFLAGS := -shared")
            targets.append("else")
            targets.append("    # Windows/MinGW")
            targets.append("    HOST_SUFFIX := .dll")
            targets.append("    HOST_LDFLAGS := -shared")
            targets.append("endif")
        else:
            # Specific platform
            if platform == 'macos':
                targets.append("# macOS build settings")
                targets.append("HOST_SUFFIX := .dylib")
                targets.append("HOST_LDFLAGS := -dynamiclib -undefined dynamic_lookup")
            elif platform == 'linux':
                targets.append("# Linux build settings")
                targets.append("HOST_SUFFIX := .so")
                targets.append("HOST_LDFLAGS := -shared")
            elif platform == 'windows':
                targets.append("# Windows build settings")
                targets.append("HOST_SUFFIX := .dll")
                targets.append("HOST_LDFLAGS := -shared")
        
        targets.append("")
        
        # Find input variable (case-insensitive search)
        input_var = None
        var_names_lower = {k.lower(): k for k in self.variables.keys()}
        for var in ['inputs', 'sources', 'srcs', 'src', 'src_files', 'source']:
            if var in var_names_lower:
                input_var = var_names_lower[var]
                break
        
        if not input_var:
            # Create a default
            targets.append("# Source files")
            targets.append("host_inputs := $(wildcard *.cpp)")
            input_var = 'host_inputs'
        else:
            targets.append("# Use existing source file list")
            input_var = f"$({input_var})"
        
        # Output files - handle different source extensions
        targets.append(f"# Transform source files to host plugins")
        if self.source_extension == 'mixed':
            targets.append("# Original sources use mixed .cpp and .cc extensions")
            targets.append(f"host_plugins := $(patsubst %.cpp,%$(HOST_SUFFIX),$(basename $(filter %.cpp," + input_var + ")))")
            targets.append(f"host_plugins += $(patsubst %.cc,%$(HOST_SUFFIX),$(basename $(filter %.cc," + input_var + ")))")
        elif self.source_extension != '.cpp':
            targets.append(f"# Original sources use {self.source_extension} extension")
            targets.append(f"host_plugins := $(patsubst %{self.source_extension},%$(HOST_SUFFIX),$(basename " + input_var + "))")
        else:
            targets.append(f"host_plugins := $(patsubst %{self.source_extension},%$(HOST_SUFFIX),$(basename " + input_var + "))")
        targets.append("")
        
        # Build rules
        targets.append("# Build rule for host plugins")
        if self.source_extension == 'mixed':
            # Need rules for both .cpp and .cc
            targets.append("%$(HOST_SUFFIX): %.cpp")
            targets.append("\t@echo \"Building host plugin: $@\"")
            targets.append("\t$(HOST_CXX) $(HOST_CXXFLAGS) $(HOST_LDFLAGS) -o $@ $<")
            targets.append("")
            targets.append("%$(HOST_SUFFIX): %.cc")
            targets.append("\t@echo \"Building host plugin: $@\"")
            targets.append("\t$(HOST_CXX) $(HOST_CXXFLAGS) $(HOST_LDFLAGS) -o $@ $<")
        elif self.source_extension in ['.cpp', '.cc', '.cxx', '.c++']:
            targets.append(f"%$(HOST_SUFFIX): %{self.source_extension}")
            targets.append("\t@echo \"Building host plugin: $@\"")
            targets.append("\t$(HOST_CXX) $(HOST_CXXFLAGS) $(HOST_LDFLAGS) -o $@ $<")
        else:
            # For non-C++ sources, provide a template
            targets.append(f"# NOTE: You'll need to adapt this rule for {self.source_extension} files")
            targets.append(f"%$(HOST_SUFFIX): %{self.source_extension}")
            targets.append("\t@echo \"ERROR: Cannot directly build host plugins from " + self.source_extension + " files\"")
            targets.append("\t@echo \"Please convert to C++ first or modify this rule\"")
            targets.append("\t@false")
        targets.append("")
        
        # Phony targets
        targets.append("# Convenience targets")
        targets.append(".PHONY: host-plugins clean-host install-host")
        targets.append("")
        targets.append("host-plugins: $(host_plugins)")
        targets.append("\t@echo \"Built $(words $(host_plugins)) host plugin(s)\"")
        targets.append("")
        targets.append("clean-host:")
        targets.append("\trm -f *.dylib *.so *.dll")
        targets.append("")
        targets.append("# Install to a test directory (customize INSTALL_DIR as needed)")
        targets.append("INSTALL_DIR ?= ../test_plugins")
        targets.append("install-host: host-plugins")
        targets.append("\t@mkdir -p $(INSTALL_DIR)")
        targets.append("\tcp $(host_plugins) $(INSTALL_DIR)/")
        targets.append("\t@echo \"Installed to $(INSTALL_DIR)\"")
        targets.append("")
        
        # Help target
        targets.append("# Help for host builds")
        targets.append("help-host:")
        targets.append("\t@echo \"Host build targets for VCV Rack emulator testing:\"")
        targets.append("\t@echo \"  make host-plugins    - Build all plugins for host platform\"")
        targets.append("\t@echo \"  make clean-host      - Remove host plugin builds\"")
        targets.append("\t@echo \"  make install-host    - Copy plugins to test directory\"")
        targets.append("\t@echo \"\"")
        targets.append("\t@echo \"Individual plugin targets:\"")
        targets.append("\t@echo \"  make <plugin>$(HOST_SUFFIX)\"")
        targets.append("")
        
        targets.append("# === End VCV Emulator Test Builds ===")
        
        return '\n'.join(targets)
    
    def augment_makefile(self, platform: str = 'auto') -> str:
        """Add host build targets to the Makefile."""
        if self.augmented:
            print("Warning: Makefile appears to already be augmented. Skipping.")
            return '\n'.join(self.lines)
        
        # Find a good insertion point (end of file or before .PHONY if it exists)
        insert_index = len(self.lines)
        
        for i in range(len(self.lines) - 1, -1, -1):
            line = self.lines[i].strip()
            if line and not line.startswith('#'):
                insert_index = i + 1
                break
        
        # Generate and insert the new targets
        new_targets = self.generate_host_targets(platform)
        augmented_lines = self.lines[:insert_index] + [new_targets] + self.lines[insert_index:]
        
        return '\n'.join(augmented_lines)


def main():
    parser = argparse.ArgumentParser(
        description='Add host platform build targets to DistingNT plugin Makefiles'
    )
    parser.add_argument(
        '--input', '-i',
        default='Makefile',
        help='Input Makefile path (default: Makefile)'
    )
    parser.add_argument(
        '--output', '-o',
        default=None,
        help='Output Makefile path (default: overwrite input)'
    )
    parser.add_argument(
        '--platform', '-p',
        choices=['auto', 'macos', 'windows', 'linux', 'all'],
        default='auto',
        help='Target platform (default: auto-detect)'
    )
    parser.add_argument(
        '--dry-run', '-n',
        action='store_true',
        help='Print augmented Makefile without writing'
    )
    
    args = parser.parse_args()
    
    # Read input file
    try:
        with open(args.input, 'r') as f:
            content = f.read()
    except FileNotFoundError:
        print(f"Error: Input file '{args.input}' not found.")
        sys.exit(1)
    
    # Process the Makefile
    augmenter = MakefileAugmenter()
    augmenter.parse_makefile(content)
    
    if augmenter.augmented:
        print(f"Info: '{args.input}' appears to already have host build targets.")
        if not args.dry_run:
            sys.exit(0)
    
    # Generate augmented content
    augmented = augmenter.augment_makefile(args.platform)
    
    # Output
    if args.dry_run:
        print(augmented)
    else:
        output_path = args.output or args.input
        
        # Backup original if overwriting
        if output_path == args.input and not args.output:
            backup_path = args.input + '.bak'
            if not os.path.exists(backup_path):
                with open(backup_path, 'w') as f:
                    f.write(content)
                print(f"Info: Original backed up to '{backup_path}'")
        
        with open(output_path, 'w') as f:
            f.write(augmented)
        
        print(f"Success: Augmented Makefile written to '{output_path}'")
        print("Use 'make host-plugins' to build plugins for your host platform")
        print("Use 'make help-host' to see all available host build targets")


if __name__ == '__main__':
    main()