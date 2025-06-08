#!/bin/bash

# Compile and run ResourceManager tests with automatic cleanup

echo "🔧 Compiling and running ResourceManager tests..."
echo "================================================"

# Change to tests directory
cd "$(dirname "$0")"

# Common compile flags
INCLUDES="-I../src -I../build/_deps/raylib-src/src -I../build/_deps/spdlog-src/include -I../build/_deps/entt-src/src -I../build/_deps/glm-src"
LIBS="-L../build -L../build/_deps/raylib-build/raylib -lraylib"
FRAMEWORKS="-framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -framework CoreFoundation"
FLAGS="-std=c++20"

# Array of test files to compile and run
declare -a tests=(
    "test_resource_manager_safety"
    "test_resource_manager_threading"
    "test_resource_manager_headless"
    "test_resource_manager_threading_fix"
    "test_resource_manager_memory_fix"
    "test_resource_manager_init_order"
    "test_resource_manager_exception_safety"
    "test_resource_manager_integration"
)

# Function to compile and run a test
run_test() {
    local test_name=$1
    echo ""
    echo "📋 Test: $test_name"
    echo "------------------------"
    
    # Compile
    echo "  Compiling..."
    if g++ $FLAGS ${test_name}.cpp ../src/resources/resource_manager.cpp $INCLUDES $LIBS $FRAMEWORKS -pthread -o $test_name 2>/dev/null; then
        echo "  ✅ Compiled successfully"
        
        # Run
        echo "  Running..."
        if ./$test_name; then
            echo "  ✅ Test passed!"
        else
            echo "  ❌ Test failed!"
            exit 1
        fi
    else
        echo "  ❌ Compilation failed!"
        exit 1
    fi
}

# Run all tests
for test in "${tests[@]}"; do
    if [[ -f "${test}.cpp" ]]; then
        run_test $test
    else
        echo "⚠️  Warning: ${test}.cpp not found, skipping..."
    fi
done

echo ""
echo "================================================"
echo "✅ All tests completed successfully!"
echo ""

# Clean up executables
echo "🧹 Cleaning up test executables..."
./clean_test_executables.sh

echo ""
echo "✨ Done!"