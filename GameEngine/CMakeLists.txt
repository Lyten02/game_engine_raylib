cmake_minimum_required(VERSION 3.20)
project(GameEngine)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# В CMakeLists.txt добавьте в начало:
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find Python3 for running tests
find_package(Python3 COMPONENTS Interpreter REQUIRED)

# Set global cache directory for dependencies
set(GLOBAL_DEPS_CACHE "${CMAKE_SOURCE_DIR}/.deps_cache" CACHE PATH "Global dependencies cache directory")
set(FETCHCONTENT_BASE_DIR "${GLOBAL_DEPS_CACHE}" CACHE PATH "FetchContent base directory")
message(STATUS "Using global dependencies cache: ${GLOBAL_DEPS_CACHE}")

# FetchContent for downloading dependencies
include(FetchContent)

# Optimize FetchContent for faster builds
set(FETCHCONTENT_QUIET FALSE)
set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)

# Download and configure raylib
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.0
    PATCH_COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/patches/raylib-cmake-fix.patch <SOURCE_DIR>/
                  COMMAND patch -p1 < raylib-cmake-fix.patch || true
)

# Download and configure spdlog
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.14.1
)

# Download and configure EnTT
FetchContent_Declare(
    EnTT
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG v3.13.2
)

# Download and configure GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)

# Download and configure nlohmann/json
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)

# Make dependencies available
FetchContent_MakeAvailable(raylib spdlog EnTT glm json)

# Lua removed - using C++ game logic system instead

# Add executable
add_executable(game_engine 
    src/main.cpp
    src/engine.cpp
    src/engine/engine_core.cpp
    src/engine/systems_manager.cpp
    src/engine/command_registry.cpp
    src/engine/command_registry_build.cpp
    src/engine/command_registry_project.cpp
    src/engine/command_registry_engine.cpp
    src/engine/command_registry_entity.cpp
    src/engine/command_registry_scene.cpp
    src/systems/render_system.cpp
    src/scene/scene.cpp
    src/resources/resource_manager.cpp
    src/console/console.cpp
    src/console/command_processor.cpp
    src/utils/file_utils.cpp
    src/utils/string_utils.cpp
    src/utils/config.cpp
    src/utils/path_utils.cpp
    src/utils/engine_paths.cpp
    src/utils/process_executor.cpp
    src/scripting/script_manager_stub.cpp
    src/scripting/game_logic_manager.cpp
    src/scripting/plugin_manager.cpp
    src/project/project.cpp
    src/project/project_manager.cpp
    src/project/project_validator.cpp
    src/serialization/scene_serializer.cpp
    src/serialization/component_registry.cpp
    src/serialization/dynamic_component_registry.cpp
    src/systems/empty_render_system.cpp
    src/systems/optional_render_system.cpp
    src/build/build_system.cpp
    src/build/build_config.cpp
    src/build/async_build_system.cpp
    src/engine/play_mode.cpp
    src/cli/cli_engine.cpp
    src/cli/cli_argument_parser.cpp
    src/packages/package.cpp
    src/packages/package_manager.cpp
    src/packages/package_loader.cpp
    src/plugins/plugin_manager.cpp
    src/plugins/plugin_api.cpp
    src/engine/command_registry_package.cpp
    src/systems/sprite_batch.cpp
    packages/physics-2d/systems/physics_system.cpp
)

# Include directories
target_include_directories(game_engine PRIVATE 
    src
    packages
)

# Link libraries
target_link_libraries(game_engine 
    raylib
    spdlog::spdlog
    EnTT::EnTT
    glm::glm
    nlohmann_json::nlohmann_json
)

# Copy assets directory to build directory
file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})

# Scripts directory removed - using C++ game logic instead

# Copy config.json to build directory
file(COPY ${CMAKE_SOURCE_DIR}/config.json DESTINATION ${CMAKE_BINARY_DIR})

# Copy templates directory to build directory
file(COPY ${CMAKE_SOURCE_DIR}/templates DESTINATION ${CMAKE_BINARY_DIR})

# Copy packages directory to build directory  
file(COPY ${CMAKE_SOURCE_DIR}/packages DESTINATION ${CMAKE_BINARY_DIR})

# Build plugins if packages exist
if(EXISTS ${CMAKE_SOURCE_DIR}/packages)
    file(GLOB PACKAGE_DIRS "${CMAKE_SOURCE_DIR}/packages/*")
    foreach(PACKAGE_DIR ${PACKAGE_DIRS})
        if(IS_DIRECTORY ${PACKAGE_DIR} AND EXISTS ${PACKAGE_DIR}/CMakeLists.txt)
            get_filename_component(PACKAGE_NAME ${PACKAGE_DIR} NAME)
            message(STATUS "Found package: ${PACKAGE_NAME}")
            add_subdirectory(${PACKAGE_DIR} packages/${PACKAGE_NAME})
        endif()
    endforeach()
endif()

# Create projects directory in build directory
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/projects)

# Include test discovery module
include(${CMAKE_SOURCE_DIR}/cmake/TestDiscovery.cmake)

# Discover all tests
discover_all_tests(${CMAKE_SOURCE_DIR}/tests)

# Create test runner script
file(WRITE ${CMAKE_BINARY_DIR}/run_tests.cmake
"# Auto-generated test runner
set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
set(GAME_ENGINE ${CMAKE_BINARY_DIR}/game_engine)
set(TEST_COUNT 0)
set(PASSED_COUNT 0)
set(FAILED_TESTS)

# Check for category filter
set(TEST_CATEGORY \$ENV{TEST_CATEGORY})

# Run Python tests
foreach(TEST_FILE ${ALL_PYTHON_TESTS})
    # Filter by category if specified
    if(TEST_CATEGORY)
        if(NOT TEST_FILE MATCHES \"/\${TEST_CATEGORY}/\")
            continue()
        endif()
    endif()
    
    math(EXPR TEST_COUNT \"\${TEST_COUNT} + 1\")
    execute_process(
        COMMAND \${PYTHON_EXECUTABLE} \${TEST_FILE}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        RESULT_VARIABLE TEST_RESULT
        OUTPUT_QUIET
        ERROR_QUIET
    )
    if(TEST_RESULT EQUAL 0)
        math(EXPR PASSED_COUNT \"\${PASSED_COUNT} + 1\")
        message(\"✅ PASS: \${TEST_FILE}\")
    else()
        list(APPEND FAILED_TESTS \${TEST_FILE})
        message(\"❌ FAIL: \${TEST_FILE}\")
    endif()
endforeach()

# Run script tests  
foreach(TEST_FILE ${ALL_SCRIPT_TESTS})
    # Filter by category if specified
    if(TEST_CATEGORY)
        if(NOT TEST_FILE MATCHES \"/\${TEST_CATEGORY}/\")
            continue()
        endif()
    endif()
    
    math(EXPR TEST_COUNT \"\${TEST_COUNT} + 1\")
    execute_process(
        COMMAND \${GAME_ENGINE} --script \${TEST_FILE}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        RESULT_VARIABLE TEST_RESULT
        OUTPUT_QUIET
        ERROR_QUIET
    )
    if(TEST_RESULT EQUAL 0)
        math(EXPR PASSED_COUNT \"\${PASSED_COUNT} + 1\")
        message(\"✅ PASS: \${TEST_FILE}\")
    else()
        list(APPEND FAILED_TESTS \${TEST_FILE})
        message(\"❌ FAIL: \${TEST_FILE}\")
    endif()
endforeach()

# Summary
if(TEST_CATEGORY)
    message(\"\\n====== \${TEST_CATEGORY} TEST SUMMARY ======\")
else()
    message(\"\\n====== TEST SUMMARY ======\")
endif()
message(\"Total tests: \${TEST_COUNT}\")
message(\"Passed: \${PASSED_COUNT}\")
message(\"Failed: \${TEST_COUNT} - \${PASSED_COUNT}\")

if(FAILED_TESTS)
    message(\"\\nFailed tests:\")
    foreach(FAILED_TEST \${FAILED_TESTS})
        message(\"  - \${FAILED_TEST}\")
    endforeach()
    message(\"\\n❌ Some tests failed!\")
    # Set exit code but don't stop execution
    set(CMAKE_SCRIPT_MODE_FILE TRUE)
    if(NOT DEFINED ENV{ALLOW_TEST_FAILURES})
        message(FATAL_ERROR \"Tests failed!\")
    endif()
endif()
")

# Add custom target for running tests
add_custom_target(test
    COMMAND ${CMAKE_COMMAND} -E echo "Running GameEngine tests..."
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/run_tests.cmake
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS game_engine
    COMMENT "Running all discovered tests"
)

# Add custom target for running tests without full builds
add_custom_target(test-fast
    COMMAND ${CMAKE_COMMAND} -E echo "Running GameEngine tests (fast mode)..."
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/run_tests.cmake
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running tests without rebuild"
)


# Create C++ test compilation script
file(WRITE ${CMAKE_BINARY_DIR}/compile_cpp_tests.cmake
"# Auto-generated C++ test compilation script
set(CMAKE_CXX_COMPILER ${CMAKE_CXX_COMPILER})
set(CMAKE_CXX_FLAGS \"-std=c++20 -stdlib=libc++\")
set(INCLUDE_DIRS \"-I${CMAKE_SOURCE_DIR}/src -I${GLOBAL_DEPS_CACHE}/raylib-src/src -I${GLOBAL_DEPS_CACHE}/spdlog-src/include -I${GLOBAL_DEPS_CACHE}/entt-src/src -I${GLOBAL_DEPS_CACHE}/glm-src -I${GLOBAL_DEPS_CACHE}/json-src/include -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk\")
file(GLOB_RECURSE ENGINE_OBJECTS \"${CMAKE_BINARY_DIR}/CMakeFiles/game_engine.dir/*.o\")
# Remove main.cpp.o to avoid multiple main() definitions
list(FILTER ENGINE_OBJECTS EXCLUDE REGEX \"main.cpp.o\")
set(LIBRARIES \"-L${CMAKE_BINARY_DIR} -L${GLOBAL_DEPS_CACHE}/raylib-build/raylib -L${GLOBAL_DEPS_CACHE}/spdlog-build -lraylib -lspdlog\")
set(FRAMEWORKS \"-framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -framework CoreFoundation\")

set(CPP_TESTS ${ALL_CPP_TESTS})
set(TEST_COUNT 0)
set(PASSED_COUNT 0)
set(FAILED_TESTS)

foreach(TEST_FILE \${CPP_TESTS})
    get_filename_component(TEST_NAME \${TEST_FILE} NAME_WE)
    set(TEST_EXECUTABLE ${CMAKE_BINARY_DIR}/\${TEST_NAME})
    
    message(\"Compiling \${TEST_NAME}...\")
    
    # Prepare compile command as a list
    set(COMPILE_CMD \${CMAKE_CXX_COMPILER})
    separate_arguments(CXX_FLAGS_LIST UNIX_COMMAND \"\${CMAKE_CXX_FLAGS}\")
    separate_arguments(INCLUDE_DIRS_LIST UNIX_COMMAND \"\${INCLUDE_DIRS}\")
    separate_arguments(LIBRARIES_LIST UNIX_COMMAND \"\${LIBRARIES}\")
    separate_arguments(FRAMEWORKS_LIST UNIX_COMMAND \"\${FRAMEWORKS}\")
    
    execute_process(
        COMMAND \${COMPILE_CMD} \${CXX_FLAGS_LIST} \${INCLUDE_DIRS_LIST} \${TEST_FILE} \${ENGINE_OBJECTS} -o \${TEST_EXECUTABLE} \${LIBRARIES_LIST} \${FRAMEWORKS_LIST}
        RESULT_VARIABLE COMPILE_RESULT
        OUTPUT_VARIABLE COMPILE_OUTPUT
        ERROR_VARIABLE COMPILE_ERROR
    )
    
    if(NOT COMPILE_RESULT EQUAL 0)
        message(\"Compilation failed for \${TEST_NAME}:\")
        message(\"STDOUT: \${COMPILE_OUTPUT}\")
        message(\"STDERR: \${COMPILE_ERROR}\")
    endif()
    
    if(COMPILE_RESULT EQUAL 0)
        math(EXPR TEST_COUNT \"\${TEST_COUNT} + 1\")
        execute_process(
            COMMAND \${TEST_EXECUTABLE}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            RESULT_VARIABLE TEST_RESULT
            OUTPUT_QUIET
            ERROR_QUIET
        )
        if(TEST_RESULT EQUAL 0)
            math(EXPR PASSED_COUNT \"\${PASSED_COUNT} + 1\")
            message(\"✅ PASS: \${TEST_NAME}\")
        else()
            list(APPEND FAILED_TESTS \${TEST_NAME})
            message(\"❌ FAIL: \${TEST_NAME}\")
        endif()
        # Clean up executable
        file(REMOVE \${TEST_EXECUTABLE})
    else()
        message(\"❌ COMPILE FAIL: \${TEST_NAME}\")
        list(APPEND FAILED_TESTS \"\${TEST_NAME} (compilation)\")
    endif()
endforeach()

# Summary
message(\"\\n====== C++ TEST SUMMARY ======\")
message(\"Total tests: \${TEST_COUNT}\")
message(\"Passed: \${PASSED_COUNT}\")
message(\"Failed: \${TEST_COUNT} - \${PASSED_COUNT}\")

if(FAILED_TESTS)
    message(\"\\nFailed tests:\")
    foreach(FAILED_TEST \${FAILED_TESTS})
        message(\"  - \${FAILED_TEST}\")
    endforeach()
    message(\"\\n❌ Some C++ tests failed!\")
    # Set exit code but don't stop execution
    set(CMAKE_SCRIPT_MODE_FILE TRUE)
    if(NOT DEFINED ENV{ALLOW_TEST_FAILURES})
        message(FATAL_ERROR \"C++ tests failed!\")
    endif()
endif()
")

# Add custom target for running C++ tests
add_custom_target(test-cpp
    COMMAND ${CMAKE_COMMAND} -E echo "Running C++ tests..."
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/compile_cpp_tests.cmake
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Compiling and running C++ tests"
)

# Create all-tests runner script
file(WRITE ${CMAKE_BINARY_DIR}/run_all_tests.cmake
"# Auto-generated all-tests runner
set(ENV{ALLOW_TEST_FAILURES} 1)

message(\"Running Python and Script tests...\")
execute_process(
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/run_tests.cmake
    RESULT_VARIABLE PYTHON_RESULT
)

message(\"\\nRunning C++ tests...\")
execute_process(
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/compile_cpp_tests.cmake
    RESULT_VARIABLE CPP_RESULT
)

message(\"\\n====== OVERALL TEST SUMMARY ======\")
if(PYTHON_RESULT EQUAL 0 AND CPP_RESULT EQUAL 0)
    message(\"✅ All test suites passed!\")
else()
    if(NOT PYTHON_RESULT EQUAL 0)
        message(\"❌ Python/Script tests had failures\")
    endif()
    if(NOT CPP_RESULT EQUAL 0)
        message(\"❌ C++ tests had failures\")
    endif()
    message(\"\\nCheck output above for detailed failure information.\")
endif()
")

# Add custom target for running all tests
add_custom_target(all-tests
    COMMAND ${CMAKE_COMMAND} -E echo "Running all tests: Python, Script, C++..."
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/run_all_tests.cmake
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS game_engine
    COMMENT "Running complete test suite"
)

# Add category-specific test targets
add_custom_target(test-unit
    COMMAND ${CMAKE_COMMAND} -E echo "Running unit tests..."
    COMMAND ${CMAKE_COMMAND} -E env TEST_CATEGORY=unit ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/run_tests.cmake
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running unit tests only"
)

add_custom_target(test-integration
    COMMAND ${CMAKE_COMMAND} -E echo "Running integration tests..."
    COMMAND ${CMAKE_COMMAND} -E env TEST_CATEGORY=integration ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/run_tests.cmake
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running integration tests only"
)

add_custom_target(test-system
    COMMAND ${CMAKE_COMMAND} -E echo "Running system tests..."
    COMMAND ${CMAKE_COMMAND} -E env TEST_CATEGORY=system ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/run_tests.cmake
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running system tests only"
)

# Add custom target for cleaning test data
add_custom_target(clean-tests
    COMMAND ${CMAKE_COMMAND} -E echo "Cleaning test output..."
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/output
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/output
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Cleaning test output directory"
)

# Add custom target for cleaning logs
add_custom_target(clean-logs
    COMMAND ${CMAKE_COMMAND} -E echo "Cleaning log files..."
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/logs
    COMMENT "Cleaning log directory"
)

# Add custom target for full clean (including test data and logs)
add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -E echo "Performing full clean..."
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean-tests
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean-logs
    COMMENT "Full clean including tests and logs"
)