#!/bin/bash

# Run C++ Test Suite with unified runner
# This script compiles and runs the C++ test runner

echo "🧪 Building C++ Test Runner..."
echo "================================================"

# Change to tests directory
cd "$(dirname "$0")"

# Common compile flags (using global deps cache)
INCLUDES="-I../src -I../.deps_cache/_deps/raylib-src/src -I../.deps_cache/_deps/spdlog-src/include -I../.deps_cache/_deps/entt-src/src -I../.deps_cache/_deps/glm-src -I../.deps_cache/_deps/json-src/include"
LIBS="-L../build -L../.deps_cache/_deps/raylib-build/raylib -L../.deps_cache/_deps/spdlog-build -lraylib -lspdlog"
FRAMEWORKS="-framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -framework CoreFoundation"
FLAGS="-std=c++20"

# Lua removed - using C++ GameLogicManager instead

# Compile the test runner
echo "Compiling test runner..."
if g++ $FLAGS run_cpp_tests.cpp cpp_test_runner.cpp $INCLUDES $LIBS $FRAMEWORKS -pthread -o cpp_test_runner 2>&1; then
    echo "✅ Test runner compiled successfully"
else
    echo "❌ Failed to compile test runner!"
    exit 1
fi

echo ""
echo "🚀 Running C++ Test Suite..."
echo "================================================"

# Pass all arguments to the test runner
./cpp_test_runner "$@"
TEST_RESULT=$?

# Clean up
echo ""
echo "🧹 Cleaning up..."
rm -f cpp_test_runner

# Exit with test result
exit $TEST_RESULT