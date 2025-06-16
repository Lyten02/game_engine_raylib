#!/bin/bash

# Smart rebuild - automatically chooses the best rebuild strategy
# Based on what has changed and what exists

echo "=== Smart Rebuild ==="
echo "Analyzing project state..."

# Function to check if source files changed
check_source_changes() {
    if [ ! -f "build/game_engine" ]; then
        return 1  # No executable, need rebuild
    fi
    
    # Check if any source file is newer than the executable
    # Using a more portable approach
    for file in $(find src -name "*.cpp" -o -name "*.h" -type f); do
        if [ "$file" -nt "build/game_engine" ]; then
            echo "→ Changed file detected: $file"
            return 1  # Source changed, need rebuild
        fi
    done
    
    return 0  # No changes
}

# Check various conditions
if [ ! -d "build" ]; then
    echo "→ No build directory found"
    echo "→ Running full rebuild..."
    ./rebuild.sh
elif [ ! -f "build/CMakeCache.txt" ]; then
    echo "→ No CMake cache found"
    echo "→ Running full rebuild..."
    ./rebuild.sh
elif [ ! -d ".deps_cache" ]; then
    echo "→ No cached dependencies found"
    echo "→ Running full rebuild to download dependencies..."
    ./rebuild.sh
elif [ "CMakeLists.txt" -nt "build/CMakeCache.txt" ]; then
    echo "→ CMakeLists.txt was modified"
    echo "→ Running fast rebuild (preserving dependencies)..."
    ./rebuild_fast.sh
elif check_source_changes; then
    echo "→ No source changes detected"
    echo "→ Build is up to date!"
    echo ""
    echo "Executable: build/game_engine"
    echo "Run with: cd build && ./game_engine"
else
    echo "→ Source files changed"
    echo "→ Running incremental rebuild..."
    ./rebuild_incremental.sh
fi