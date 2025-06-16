#!/bin/bash

# Full clean rebuild - removes everything including cached dependencies
# Use this when you want a completely fresh build

echo "=== Full Clean Rebuild ==="
echo "This will remove all cached dependencies and rebuild everything from scratch."
echo "For faster rebuilds, use:"
echo "  ./rebuild_fast.sh       - Preserves cached dependencies"
echo "  ./rebuild_incremental.sh - Only recompiles changed files"
echo ""

echo "Cleaning build directory..."
rm -rf build
mkdir build
cd build

echo "Running CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

echo "Building project..."
time make -j8

if [ $? -eq 0 ]; then
    echo ""
    echo "Build complete!"
    echo "Executable: build/game_engine"
    echo "Run with: cd build && ./game_engine"
    echo ""
    echo "TIP: Next time use ./rebuild_incremental.sh for faster builds!"
else
    echo ""
    echo "Build failed!"
    exit 1
fi