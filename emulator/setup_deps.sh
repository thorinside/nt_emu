#!/bin/bash

# Setup script for disting NT emulator dependencies

echo "Setting up dependencies for Disting NT Emulator..."

# Create third_party directory
mkdir -p third_party

# Download Dear ImGui
if [ ! -d "third_party/imgui" ]; then
    echo "Downloading Dear ImGui..."
    cd third_party
    git clone --depth 1 --branch v1.90.1 https://github.com/ocornut/imgui.git
    cd ..
else
    echo "ImGui already exists"
fi

# Download json library
if [ ! -d "third_party/json" ]; then
    echo "Downloading nlohmann/json..."
    cd third_party
    git clone --depth 1 --branch v3.11.3 https://github.com/nlohmann/json.git
    cd ..
else
    echo "JSON library already exists"
fi

# Download spdlog
if [ ! -d "third_party/spdlog" ]; then
    echo "Downloading spdlog..."
    cd third_party
    git clone --depth 1 --branch v1.12.0 https://github.com/gabime/spdlog.git
    cd ..
else
    echo "spdlog already exists"
fi

echo "Dependencies setup complete!"
echo "You can now run: cd build && cmake .. && make"