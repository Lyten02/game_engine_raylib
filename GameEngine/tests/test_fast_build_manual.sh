#!/bin/bash
# Manual test for fast build functionality

echo "=== Fast Build Manual Test ==="
cd ../build

# Clean up any previous test
rm -rf projects/FastBuildManual output/FastBuildManual 2>/dev/null

# Create and build project (full build)
echo "1. Creating project and doing full build..."
time ./game_engine --script - <<EOF
project.create FastBuildManual
project.open FastBuildManual
scene.create main
entity.create Player
project.build
EOF

# Check if build succeeded
if [ ! -f "output/FastBuildManual/game" ]; then
    echo "ERROR: Full build failed - executable not found"
    exit 1
fi

echo "Full build completed successfully!"

# Clean build output but keep dependencies
echo "2. Cleaning build output (keeping dependencies)..."
rm -rf output/FastBuildManual/build/CMakeFiles
rm -rf output/FastBuildManual/build/cmake_install.cmake
rm -rf output/FastBuildManual/build/Makefile
rm -rf output/FastBuildManual/build/CMakeCache.txt
rm -f output/FastBuildManual/game

# Fast rebuild
echo "3. Running fast build..."
time ./game_engine --script - <<EOF
project.open FastBuildManual
project.build.fast
EOF

# Check if fast build succeeded
if [ ! -f "output/FastBuildManual/game" ]; then
    echo "ERROR: Fast build failed - executable not found"
    exit 1
fi

echo "Fast build completed successfully!"
echo "✓ Fast build test PASSED"