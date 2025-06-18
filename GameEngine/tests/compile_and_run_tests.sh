#!/bin/bash

# Compile and run ResourceManager tests with automatic cleanup

echo "🔧 Compiling and running ResourceManager tests..."
echo "================================================"

# Change to tests directory
cd "$(dirname "$0")"

# Common compile flags
INCLUDES="-I../src -I../.deps_cache/raylib-src/src -I../.deps_cache/spdlog-src/include -I../.deps_cache/entt-src/src -I../.deps_cache/glm-src -I../.deps_cache/json-src/include"
LIBS="-L../build -L../.deps_cache/raylib-build/raylib -L../.deps_cache/spdlog-build -lraylib -lspdlog"
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
    "test_resource_manager_exception_safety"
    "test_resource_manager_simple"
    "test_async_build_threading"
    "test_async_build_thread_safety"
    "test_default_texture_manager"
    "test_call_once_retry_behavior"
    "test_memory_ordering"
    "test_exception_safety"
    "test_engine_init"
    "test_console_autocompletion"
    "test_config_depth"
    "test_log_limiter_generic_keys"
    "test_script_manager_null_safety"
    "test_resource_functionality"
    "test_resource_memory"
    "test_resource_pointer_consistency"
    "test_resource_simple"
    "test_build_system_basic"
    "test_render_system_null_pointer"
    "test_resource_manager_exception_deadlock"
    "test_sprite_batch"
    "test_sprite_batch_rendering"
    "test_render_system_batching"
    "test_package_manager"
    "test_package_metadata"
    "test_package_version_validation"
    "test_package_loader"
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
            ../src/utils/path_utils.cpp \
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
    elif [[ "$test_name" == "test_async_build_threading" ]] || [[ "$test_name" == "test_async_build_thread_safety" ]]; then
        # Async build system test
        if g++ $FLAGS ${test_name}.cpp \
            ../src/build/async_build_system.cpp \
            ../src/build/build_system.cpp \
            ../src/build/build_config.cpp \
            ../src/project/project.cpp \
            ../src/utils/file_utils.cpp \
            ../src/utils/string_utils.cpp \
            ../src/utils/log_limiter.cpp \
            ../src/utils/engine_paths.cpp \
            ../src/utils/process_executor.cpp \
            $INCLUDES $LIBS $FRAMEWORKS -pthread -o $test_name 2>/dev/null; then
            echo "  ✅ Compiled successfully"
        else
            echo "  ❌ Compilation failed!"
            ((failed_tests++))
            failed_test_names+=("$test_name (compilation)")
            return
        fi
    elif [[ "$test_name" == "test_console_autocompletion" ]]; then
        # Console autocompletion test
        if g++ $FLAGS ${test_name}.cpp \
            ../src/console/console.cpp \
            ../src/console/command_processor.cpp \
            ../src/utils/config.cpp \
            $INCLUDES $LIBS $FRAMEWORKS -pthread -o $test_name 2>/dev/null; then
            echo "  ✅ Compiled successfully"
        else
            echo "  ❌ Compilation failed!"
            ((failed_tests++))
            failed_test_names+=("$test_name (compilation)")
            return
        fi
    elif [[ "$test_name" == "test_config_depth" ]]; then
        # Config depth test
        if g++ $FLAGS ${test_name}.cpp \
            ../src/utils/config.cpp \
            $INCLUDES $LIBS $FRAMEWORKS -pthread -o $test_name 2>/dev/null; then
            echo "  ✅ Compiled successfully"
        else
            echo "  ❌ Compilation failed!"
            ((failed_tests++))
            failed_test_names+=("$test_name (compilation)")
            return
        fi
    elif [[ "$test_name" == "test_log_limiter_generic_keys" ]]; then
        # Log limiter test (header-only, no ResourceManager dependency)
        if g++ $FLAGS ${test_name}.cpp \
            -I../src -I../build/_deps/spdlog-src/include -L../build/_deps/spdlog-build -lspdlog -pthread -o $test_name 2>&1; then
            echo "  ✅ Compiled successfully"
        else
            echo "  ❌ Compilation failed!"
            ((failed_tests++))
            failed_test_names+=("$test_name (compilation)")
            return
        fi
    elif [[ "$test_name" == "test_script_manager_null_safety" ]]; then
        # Script manager test
        if g++ $FLAGS ${test_name}.cpp \
            ../src/scripting/script_manager.cpp \
            ../src/scripting/lua_bindings.cpp \
            ../src/utils/file_utils.cpp \
            $INCLUDES $LIBS $FRAMEWORKS -pthread $LUA_LIBS -o $test_name 2>/dev/null; then
            echo "  ✅ Compiled successfully"
        else
            echo "  ❌ Compilation failed!"
            ((failed_tests++))
            failed_test_names+=("$test_name (compilation)")
            return
        fi
    elif [[ "$test_name" == "test_resource_functionality" ]] || [[ "$test_name" == "test_resource_memory" ]] || [[ "$test_name" == "test_resource_pointer_consistency" ]] || [[ "$test_name" == "test_resource_simple" ]] || [[ "$test_name" == "test_resource_manager_exception_safety" ]] || [[ "$test_name" == "test_resource_manager_simple" ]]; then
        # Resource tests (various)
        if g++ $FLAGS ${test_name}.cpp \
            ../src/resources/resource_manager.cpp \
            $INCLUDES $LIBS $FRAMEWORKS -pthread -o $test_name 2>&1; then
            echo "  ✅ Compiled successfully"
        else
            echo "  ❌ Compilation failed!"
            ((failed_tests++))
            failed_test_names+=("$test_name (compilation)")
            return
        fi
    elif [[ "$test_name" == "test_build_system_basic" ]]; then
        # Build system test
        if g++ $FLAGS ${test_name}.cpp \
            ../src/build/build_system.cpp \
            ../src/build/async_build_system.cpp \
            ../src/build/build_config.cpp \
            ../src/project/project.cpp \
            ../src/utils/file_utils.cpp \
            ../src/utils/string_utils.cpp \
            ../src/utils/path_utils.cpp \
            $INCLUDES $LIBS $FRAMEWORKS -pthread -o $test_name 2>/dev/null; then
            echo "  ✅ Compiled successfully"
        else
            echo "  ❌ Compilation failed!"
            ((failed_tests++))
            failed_test_names+=("$test_name (compilation)")
            return
        fi
    elif [[ "$test_name" == "test_default_texture_manager" ]]; then
        # Default texture manager test
        if g++ $FLAGS ${test_name}.cpp \
            ../src/resources/resource_manager.cpp \
            $INCLUDES $LIBS $FRAMEWORKS -pthread -o $test_name 2>&1; then
            echo "  ✅ Compiled successfully"
        else
            echo "  ❌ Compilation failed!"
            ((failed_tests++))
            failed_test_names+=("$test_name (compilation)")
            return
        fi
    elif [[ "$test_name" == "test_call_once_retry_behavior" ]]; then
        # Thread-safe initialization test
        if g++ $FLAGS ${test_name}.cpp \
            ../src/resources/resource_manager.cpp \
            $INCLUDES $LIBS $FRAMEWORKS -pthread -o $test_name 2>&1; then
            echo "  ✅ Compiled successfully"
        else
            echo "  ❌ Compilation failed!"
            ((failed_tests++))
            failed_test_names+=("$test_name (compilation)")
            return
        fi
    elif [[ "$test_name" == "test_render_system_null_pointer" ]]; then
        # Render system null pointer test
        if g++ $FLAGS ${test_name}.cpp \
            ../src/systems/render_system.cpp \
            ../src/resources/resource_manager.cpp \
            $INCLUDES $LIBS $FRAMEWORKS -pthread -o $test_name 2>&1; then
            echo "  ✅ Compiled successfully"
        else
            echo "  ❌ Compilation failed!"
            ((failed_tests++))
            failed_test_names+=("$test_name (compilation)")
            return
        fi
    elif [[ "$test_name" == "test_resource_manager_exception_deadlock" ]]; then
        # Resource manager exception safety test
        if g++ $FLAGS ${test_name}.cpp \
            ../src/resources/resource_manager.cpp \
            $INCLUDES $LIBS $FRAMEWORKS -pthread -o $test_name 2>&1; then
            echo "  ✅ Compiled successfully"
        else
            echo "  ❌ Compilation failed!"
            ((failed_tests++))
            failed_test_names+=("$test_name (compilation)")
            return
        fi
    elif [[ "$test_name" == "test_sprite_batch" ]] || [[ "$test_name" == "test_sprite_batch_rendering" ]]; then
        # Sprite batch test
        if g++ $FLAGS ${test_name}.cpp \
            ../src/render/sprite_batch.cpp \
            $INCLUDES $LIBS $FRAMEWORKS -pthread -o $test_name 2>&1; then
            echo "  ✅ Compiled successfully"
        else
            echo "  ❌ Compilation failed!"
            ((failed_tests++))
            failed_test_names+=("$test_name (compilation)")
            return
        fi
    elif [[ "$test_name" == "test_render_system_batching" ]]; then
        # Render system batching test
        if g++ $FLAGS ${test_name}.cpp \
            ../src/systems/render_system.cpp \
            ../src/render/sprite_batch.cpp \
            ../src/resources/resource_manager.cpp \
            $INCLUDES $LIBS $FRAMEWORKS -pthread -o $test_name 2>&1; then
            echo "  ✅ Compiled successfully"
        else
            echo "  ❌ Compilation failed!"
            ((failed_tests++))
            failed_test_names+=("$test_name (compilation)")
            return
        fi
    elif [[ "$test_name" == "test_package_manager" ]] || [[ "$test_name" == "test_package_metadata" ]] || [[ "$test_name" == "test_package_version_validation" ]]; then
        # Package manager tests
        if g++ $FLAGS ${test_name}.cpp \
            ../src/packages/package.cpp \
            ../src/packages/package_manager.cpp \
            $INCLUDES $LIBS $FRAMEWORKS -pthread -o $test_name 2>&1; then
            echo "  ✅ Compiled successfully"
        else
            echo "  ❌ Compilation failed!"
            ((failed_tests++))
            failed_test_names+=("$test_name (compilation)")
            return
        fi
    elif [[ "$test_name" == "test_package_loader" ]]; then
        # Package loader test with physics system
        if g++ $FLAGS ${test_name}.cpp \
            ../src/packages/package.cpp \
            ../src/packages/package_manager.cpp \
            ../src/packages/package_loader.cpp \
            ../packages/physics-2d/systems/physics_system.cpp \
            -I../packages \
            $INCLUDES $LIBS $FRAMEWORKS -pthread -o $test_name 2>&1; then
            echo "  ✅ Compiled successfully"
        else
            echo "  ❌ Compilation failed!"
            ((failed_tests++))
            failed_test_names+=("$test_name (compilation)")
            return
        fi
    else
        # Resource manager tests
        if g++ $FLAGS ${test_name}.cpp ../src/resources/resource_manager.cpp $INCLUDES $LIBS $FRAMEWORKS -pthread -o $test_name 2>&1; then
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

# Clean up test projects
echo "🧹 Cleaning up old test projects..."
cleanup_test_projects() {
    local test_projects_dir="test_projects"
    if [ -d "$test_projects_dir" ]; then
        # Keep only the 5 most recent test projects
        # macOS compatible version
        ls -t1d "$test_projects_dir"/TestProject* 2>/dev/null | tail -n +6 | xargs -r rm -rf 2>/dev/null || true
    fi
}
cleanup_test_projects

echo ""

# Exit with appropriate code
if [ $failed_tests -eq 0 ]; then
    echo "✨ All tests passed!"
    exit 0
else
    echo "❌ Some tests failed!"
    exit 1
fi