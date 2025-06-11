#!/bin/bash
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
