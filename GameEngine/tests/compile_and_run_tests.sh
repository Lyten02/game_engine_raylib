#!/bin/bash

# Compile and run ResourceManager tests with automatic cleanup

echo "🔧 Compiling and running ResourceManager tests..."
echo "================================================"

# Change to tests directory
cd "$(dirname "$0")"

# Common compile flags
INCLUDES="-I../src -I../build/_deps/raylib-src/src -I../build/_deps/spdlog-src/include -I../build/_deps/entt-src/src -I../build/_deps/glm-src -I../build/_deps/json-src/include"
LIBS="-L../build -L../build/_deps/raylib-build/raylib -L../build/_deps/spdlog-build -lraylib -lspdlog"
FRAMEWORKS="-framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -framework CoreFoundation"
FLAGS="-std=c++20"

# Add Lua paths
LUA_CFLAGS=$(pkg-config --cflags lua 2>/dev/null || echo "")
LUA_LIBS=$(pkg-config --libs lua 2>/dev/null || echo "-llua")
INCLUDES="$INCLUDES $LUA_CFLAGS"

# Array of test files to compile and run
declare -a tests=(
    "test_resource_manager_safety"
    "test_resource_manager_threading"
    "test_resource_manager_headless"
    "test_resource_manager_memory"
    "test_resource_manager_threading_fix"
    "test_resource_manager_memory_fix"
    "test_resource_manager_init_order"
    "test_resource_manager_integration"
    "test_async_build_threading"  # Add this line
    "test_default_texture_manager"
    "test_call_once_retry_behavior"
    "test_memory_ordering"
    "test_exception_safety"
    "test_engine_init"
)

# Track test results
total_tests=0
passed_tests=0
failed_tests=0
failed_test_names=()

# Function to compile and run a test
run_test() {
    local test_name=$1
    echo ""
    echo "📋 Test: $test_name"
    echo "------------------------"
    
    ((total_tests++))
    
    # Compile
    echo "  Compiling..."
    
    # Different compilation for different test types
    if [[ "$test_name" == "test_engine_init" ]]; then
        # Engine test needs more source files
        if g++ $FLAGS ${test_name}.cpp \
            ../src/engine.cpp \
            ../src/engine/engine_core.cpp \
            ../src/engine/systems_manager.cpp \
            ../src/engine/command_registry.cpp \
            ../src/engine/command_registry_build.cpp \
            ../src/engine/command_registry_project.cpp \
            ../src/engine/command_registry_engine.cpp \
            ../src/engine/command_registry_entity.cpp \
            ../src/engine/command_registry_scene.cpp \
            ../src/systems/render_system.cpp \
            ../src/scene/scene.cpp \
            ../src/resources/resource_manager.cpp \
            ../src/console/console.cpp \
            ../src/console/command_processor.cpp \
            ../src/utils/file_utils.cpp \
            ../src/utils/string_utils.cpp \
            ../src/utils/config.cpp \
            ../src/scripting/script_manager.cpp \
            ../src/scripting/lua_bindings.cpp \
            ../src/project/project.cpp \
            ../src/project/project_manager.cpp \
            ../src/project/project_validator.cpp \
            ../src/serialization/scene_serializer.cpp \
            ../src/serialization/component_registry.cpp \
            ../src/build/build_system.cpp \
            ../src/build/build_config.cpp \
            ../src/build/async_build_system.cpp \
            ../src/engine/play_mode.cpp \
            $INCLUDES $LIBS $FRAMEWORKS -pthread $LUA_LIBS -o $test_name 2>/dev/null; then
            echo "  ✅ Compiled successfully"
        else
            echo "  ❌ Compilation failed!"
            ((failed_tests++))
            failed_test_names+=("$test_name (compilation)")
            return
        fi
    elif [[ "$test_name" == "test_async_build_threading" ]]; then
        # Async build system test
        if g++ $FLAGS ${test_name}.cpp \
            ../src/build/async_build_system.cpp \
            ../src/build/build_system.cpp \
            ../src/build/build_config.cpp \
            ../src/project/project.cpp \
            ../src/utils/file_utils.cpp \
            ../src/utils/string_utils.cpp \
            $INCLUDES $LIBS $FRAMEWORKS -pthread -o $test_name 2>/dev/null; then
            echo "  ✅ Compiled successfully"
        else
            echo "  ❌ Compilation failed!"
            ((failed_tests++))
            failed_test_names+=("$test_name (compilation)")
            return
        fi
    elif [[ "$test_name" == "test_default_texture_manager" ]] || [[ "$test_name" == "test_call_once_retry_behavior" ]]; then
        # These tests appear to be outdated or require special handling
        echo "  ⚠️  Skipping $test_name - needs update"
        ((failed_tests++))
        failed_test_names+=("$test_name (skipped - needs update)")
        return
    else
        # Resource manager tests
        if g++ $FLAGS ${test_name}.cpp ../src/resources/resource_manager.cpp $INCLUDES $LIBS $FRAMEWORKS -pthread -o $test_name 2>/dev/null; then
            echo "  ✅ Compiled successfully"
        else
            echo "  ❌ Compilation failed!"
            ((failed_tests++))
            failed_test_names+=("$test_name (compilation)")
            return
        fi
    fi
    
    # Run
    echo "  Running..."
    if ./$test_name; then
        echo "  ✅ Test passed!"
        ((passed_tests++))
    else
        echo "  ❌ Test failed!"
        ((failed_tests++))
        failed_test_names+=("$test_name (runtime)")
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
echo "TEST SUMMARY"
echo "================================================"
echo "Total tests: $total_tests"
echo "✅ Passed: $passed_tests"
echo "❌ Failed: $failed_tests"

if [ ${#failed_test_names[@]} -gt 0 ]; then
    echo ""
    echo "Failed tests:"
    for failed_test in "${failed_test_names[@]}"; do
        echo "  - $failed_test"
    done
fi

echo ""

# Clean up executables
echo "🧹 Cleaning up test executables..."
./clean_test_executables.sh

echo ""

# Exit with appropriate code
if [ $failed_tests -eq 0 ]; then
    echo "✨ All tests passed!"
    exit 0
else
    echo "❌ Some tests failed!"
    exit 1
fi