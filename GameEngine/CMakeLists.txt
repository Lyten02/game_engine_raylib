cmake_minimum_required(VERSION 3.20)
project(GameEngine)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# FetchContent for downloading dependencies
include(FetchContent)

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

# Make dependencies available
FetchContent_MakeAvailable(raylib spdlog EnTT glm)

# Add executable
add_executable(game 
    src/main.cpp
    src/engine.cpp
    src/systems/render_system.cpp
    src/scene/scene.cpp
    src/resources/resource_manager.cpp
    src/console/console.cpp
    src/console/command_processor.cpp
    src/utils/file_utils.cpp
    src/utils/string_utils.cpp
)

# Include directories
target_include_directories(game PRIVATE src)

# Link libraries
target_link_libraries(game 
    raylib
    spdlog::spdlog
    EnTT::EnTT
    glm::glm
)

# Copy assets directory to build directory
file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})