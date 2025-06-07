#!/bin/bash

# Clean test executables script
# This script removes compiled test executables to keep the directory clean

echo "🧹 Cleaning test executables..."

# Get the script directory
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Clean test executables in tests directory
echo "  Checking tests directory..."
cd "$SCRIPT_DIR"
found=0
for file in test_*; do
    # Check if file exists and is executable but not a script file
    if [[ -x "$file" && ! "$file" =~ \.(sh|py|txt|cpp|h|md)$ ]]; then
        echo "  Removing: tests/$file"
        rm -f "$file"
        found=$((found + 1))
    fi
done

# Clean test executables in build directory
if [ -d "$PROJECT_ROOT/build" ]; then
    echo "  Checking build directory..."
    cd "$PROJECT_ROOT/build"
    for file in test_*; do
        # Check if file exists and is executable but not a script file
        if [[ -x "$file" && ! "$file" =~ \.(sh|py|txt|cpp|h|md)$ ]]; then
            echo "  Removing: build/$file"
            rm -f "$file"
            found=$((found + 1))
        fi
    done
fi

if [ $found -eq 0 ]; then
    echo "  No test executables found to clean"
else
    echo "✅ Cleaned $found test executable(s)"
fi

# Also remove any .dSYM directories (macOS debug symbols)
cd "$SCRIPT_DIR"
if ls *.dSYM 1> /dev/null 2>&1; then
    echo "  Removing debug symbol directories..."
    rm -rf *.dSYM
fi

# Check build directory for .dSYM
if [ -d "$PROJECT_ROOT/build" ]; then
    cd "$PROJECT_ROOT/build"
    if ls *.dSYM 1> /dev/null 2>&1; then
        echo "  Removing debug symbol directories in build..."
        rm -rf *.dSYM
    fi
fi

echo "✨ Test directory cleanup complete!"