#!/bin/bash

# Fast rebuild script with dependency caching
# This script preserves downloaded dependencies and only rebuilds the main code

echo "=== Fast Rebuild with Cached Dependencies ==="

# Check if dependencies exist in the correct location
if [ ! -d ".deps_cache" ]; then
    echo "No cached dependencies found. Running full rebuild..."
    ./rebuild.sh
    exit 0
fi

echo "Found cached dependencies in .deps_cache/"

# If build exists and has CMakeCache, just clean game_engine artifacts
if [ -f "build/CMakeCache.txt" ]; then
    echo "Found existing CMake configuration"
    cd build
    # Remove only game_engine specific files
    rm -f game_engine
    find CMakeFiles/game_engine.dir -name "*.o" -delete 2>/dev/null || true
else
    # No CMake cache, need to configure
    echo "No CMake cache found, configuring..."
    mkdir -p build
    cd build
fi

# Configure if needed (CMake will skip if already configured)
echo "Running CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

echo "Building project..."
make -j8 game_engine

echo "Fast rebuild complete!"
echo ""
echo "Executable: build/game_engine"
echo "Run with: cd build && ./game_engine"