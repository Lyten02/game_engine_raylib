#!/bin/bash

# Fast rebuild script with dependency caching
# This script preserves downloaded dependencies and only rebuilds the main code

echo "=== Fast Rebuild with Cached Dependencies ==="

# Check if dependencies exist
if [ -d "build/_deps" ]; then
    echo "Found cached dependencies, preserving them..."
    
    # Create temp directory for deps
    mkdir -p .deps_cache
    cp -r build/_deps .deps_cache/
    
    # Clean everything except deps
    find build -mindepth 1 -maxdepth 1 ! -name '_deps' -exec rm -rf {} +
    
    # Clean only game executable and CMake files
    rm -f build/game
    rm -f build/CMakeCache.txt
    rm -rf build/CMakeFiles
    rm -f build/Makefile
    rm -f build/cmake_install.cmake
    
    echo "Cleaned build directory (dependencies preserved)"
else
    echo "No cached dependencies found, will download them..."
    mkdir -p build
fi

cd build

echo "Running CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

echo "Building project..."
make -j8 game

# If we had cached deps, restore them
if [ -d "../.deps_cache/_deps" ]; then
    rm -rf ../deps_cache
fi

echo "Fast rebuild complete!"
echo ""
echo "Executable: build/game"
echo "Run with: cd build && ./game"