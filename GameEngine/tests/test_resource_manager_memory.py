#!/usr/bin/env python3
"""
Test for ResourceManager memory efficiency improvements.
Verifies that missing textures don't grow the map and memory is managed correctly.
"""

import os
import sys
import subprocess

def run_test():
    """Run the ResourceManager memory test by compiling and executing the C++ test"""
    print("Running ResourceManager memory efficiency test...")
    
    # Get test directory
    test_dir = os.path.dirname(os.path.abspath(__file__))
    
    # Check if C++ test exists
    cpp_test_path = os.path.join(test_dir, "test_resource_memory.cpp")
    if not os.path.exists(cpp_test_path):
        print(f"Error: {cpp_test_path} not found")
        return False
    
    # Run compile_and_run_tests.sh for just this test
    compile_script = os.path.join(test_dir, "compile_and_run_tests.sh")
    if not os.path.exists(compile_script):
        print(f"Error: {compile_script} not found")
        return False
    
    # Create a temporary script that only runs our test
    temp_script = os.path.join(test_dir, "temp_run_single_test.sh")
    try:
        with open(temp_script, 'w') as f:
            f.write("""#!/bin/bash
cd "$(dirname "$0")"

# Common compile flags
# Check if we have a deps cache
if [ -d "../.deps_cache/_deps" ]; then
    DEPS_DIR="../.deps_cache/_deps"
elif [ -d "../build/_deps" ]; then
    DEPS_DIR="../build/_deps"
else
    echo "Error: Cannot find dependencies directory"
    exit 1
fi

INCLUDES="-I../src -I$DEPS_DIR/raylib-src/src -I$DEPS_DIR/spdlog-src/include -I$DEPS_DIR/entt-src/src -I$DEPS_DIR/glm-src -I$DEPS_DIR/json-src/include"
LIBS="-L../build -L$DEPS_DIR/raylib-build/raylib -L$DEPS_DIR/spdlog-build -lraylib -lspdlog"
FRAMEWORKS="-framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -framework CoreFoundation"
FLAGS="-std=c++20"

# Compile
echo "Compiling test_resource_memory..."
if g++ $FLAGS test_resource_memory.cpp ../src/resources/resource_manager.cpp $INCLUDES $LIBS $FRAMEWORKS -pthread -o test_resource_memory_executable 2>&1; then
    echo "Compilation successful"
else
    echo "Compilation failed!"
    exit 1
fi

# Run
echo "Running test..."
if ./test_resource_memory_executable; then
    echo "Test passed!"
    rm -f test_resource_memory_executable
    exit 0
else
    echo "Test failed!"
    rm -f test_resource_memory_executable
    exit 1
fi
""")
        
        # Make script executable
        os.chmod(temp_script, 0o755)
        
        # Run the script
        result = subprocess.run([temp_script], capture_output=True, text=True, timeout=30)
        
        print("Test output:")
        print(result.stdout)
        if result.stderr:
            print("Error output:")
            print(result.stderr)
        
        return result.returncode == 0
        
    finally:
        # Clean up temp script
        if os.path.exists(temp_script):
            os.remove(temp_script)

if __name__ == "__main__":
    success = run_test()
    sys.exit(0 if success else 1)