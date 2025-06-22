#!/bin/bash

# Script to format all C++ source files in the project
# Can be used in CI/CD or locally

echo "=== Auto-formatting C++ source files ==="

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$SCRIPT_DIR/.."

# Change to project root
cd "$PROJECT_ROOT"

# Check if clang-format is available
if command -v clang-format &> /dev/null; then
    echo "Using clang-format for formatting"
    
    # Find and format all C++ files
    find src -name "*.cpp" -o -name "*.h" -o -name "*.hpp" | while read file; do
        echo "Formatting: $file"
        clang-format -i "$file"
    done
    
    echo "✅ Formatting complete with clang-format"
else
    echo "clang-format not found, using Python formatter"
    
    # Use our Python formatter
    if [ -f "$SCRIPT_DIR/auto_format.py" ]; then
        python3 "$SCRIPT_DIR/auto_format.py" src
        echo "✅ Formatting complete with Python formatter"
    else
        echo "❌ Neither clang-format nor auto_format.py found"
        exit 1
    fi
fi

echo ""
echo "Formatting complete! Check git diff to see changes."