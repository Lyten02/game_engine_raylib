#!/bin/bash

# Incremental rebuild - only recompiles changed files
# Fastest option when you're making small changes

echo "=== Incremental Rebuild ==="

if [ ! -d "build" ]; then
    echo "No build directory found. Running full rebuild..."
    ./rebuild.sh
    exit 0
fi

if [ ! -f "build/CMakeCache.txt" ]; then
    echo "No CMake cache found. Running full rebuild..."
    ./rebuild.sh
    exit 0
fi

cd build

# Check if CMakeLists.txt was modified
if [ "../CMakeLists.txt" -nt "CMakeCache.txt" ]; then
    echo "CMakeLists.txt was modified, reconfiguring..."
    cmake ..
fi

echo "Building project (incremental)..."
time make -j8 game

if [ $? -eq 0 ]; then
    echo ""
    echo "Incremental build complete!"
    echo "Executable: build/game"
    echo "Run with: cd build && ./game"
else
    echo ""
    echo "Build failed! Try running ./rebuild.sh for a clean build."
fi