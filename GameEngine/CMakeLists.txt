cmake_minimum_required(VERSION 3.20)
project(GameEngine)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# В CMakeLists.txt добавьте в начало:
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
    src/render/sprite_batch.cpp
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
)

# Include directories
target_include_directories(game_engine PRIVATE 
    src
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

# Add custom target for running tests (full version by default)
add_custom_target(test
    COMMAND ${CMAKE_COMMAND} -E echo "Running GameEngine tests - including full builds..."
    COMMAND python3 ${CMAKE_SOURCE_DIR}/tests/run_all_tests.py
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS game_engine
    COMMENT "Running all tests including full builds"
)

# Add custom target for running tests without full builds
add_custom_target(test-fast
    COMMAND ${CMAKE_COMMAND} -E echo "Running GameEngine tests - skipping full builds..."
    COMMAND python3 ${CMAKE_SOURCE_DIR}/tests/run_all_tests.py --skip-full-build
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS game_engine
    COMMENT "Running tests without full builds"
)

# Add custom target for parallel tests
add_custom_target(test-parallel
    COMMAND ${CMAKE_COMMAND} -E echo "Running GameEngine tests in parallel mode..."
    COMMAND python3 ${CMAKE_SOURCE_DIR}/tests/run_all_tests.py --parallel --skip-full-build
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS game_engine
    COMMENT "Running tests in parallel"
)

# Add custom target for parallel tests with custom worker count
add_custom_target(test-parallel-2
    COMMAND ${CMAKE_COMMAND} -E echo "Running GameEngine tests in parallel mode - 2 workers..."
    COMMAND python3 ${CMAKE_SOURCE_DIR}/tests/run_all_tests.py --parallel --skip-full-build --workers 2
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS game_engine
    COMMENT "Running tests in parallel with 2 workers"
)

add_custom_target(test-parallel-4
    COMMAND ${CMAKE_COMMAND} -E echo "Running GameEngine tests in parallel mode - 4 workers..."
    COMMAND python3 ${CMAKE_SOURCE_DIR}/tests/run_all_tests.py --parallel --skip-full-build --workers 4
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS game_engine
    COMMENT "Running tests in parallel with 4 workers"
)

add_custom_target(test-parallel-8
    COMMAND ${CMAKE_COMMAND} -E echo "Running GameEngine tests in parallel mode - 8 workers..."
    COMMAND python3 ${CMAKE_SOURCE_DIR}/tests/run_all_tests.py --parallel --skip-full-build --workers 8
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS game_engine
    COMMENT "Running tests in parallel with 8 workers"
)

# Add custom target for running C++ tests (old version)
add_custom_target(test-cpp-old
    COMMAND ${CMAKE_COMMAND} -E echo "Running C++ tests (old system)..."
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/compile_and_run_tests.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests
    COMMENT "Compiling and running C++ tests (old system)"
)

# Add custom target for running C++ tests (new unified runner)
add_custom_target(test-cpp
    COMMAND ${CMAKE_COMMAND} -E echo "Running C++ tests..."
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/run_cpp_test_suite.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests
    COMMENT "Running C++ tests with unified runner"
)

# Add custom target for running C++ tests in parallel
add_custom_target(test-cpp-parallel
    COMMAND ${CMAKE_COMMAND} -E echo "Running C++ tests in parallel..."
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/run_cpp_test_suite.sh --parallel
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests
    COMMENT "Running C++ tests in parallel mode"
)

# Add custom target for running C++ tests with custom worker count
add_custom_target(test-cpp-parallel-2
    COMMAND ${CMAKE_COMMAND} -E echo "Running C++ tests with 2 workers..."
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/run_cpp_test_suite.sh --parallel --workers 2
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests
    COMMENT "Running C++ tests with 2 workers"
)

add_custom_target(test-cpp-parallel-4
    COMMAND ${CMAKE_COMMAND} -E echo "Running C++ tests with 4 workers..."
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/run_cpp_test_suite.sh --parallel --workers 4
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests
    COMMENT "Running C++ tests with 4 workers"
)


add_custom_target(test-cpp-parallel-8
    COMMAND ${CMAKE_COMMAND} -E echo "Running C++ tests with 8 workers..."
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/run_cpp_test_suite.sh --parallel --workers 8
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests
    COMMENT "Running C++ tests with 8 workers"
)

# Add custom target for cleaning test data
add_custom_target(clean-tests
    COMMAND ${CMAKE_COMMAND} -E echo "Cleaning test data..."
    COMMAND python3 ${CMAKE_SOURCE_DIR}/tests/clean_test_data.py
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/clean_test_executables.sh
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Cleaning test projects and executables"
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