cmake_minimum_required(VERSION 3.20)
project(platformer-example VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages (assume they are available from parent build)
if(NOT TARGET raylib)
    find_package(raylib REQUIRED)
endif()
if(NOT TARGET EnTT::EnTT)
    find_package(EnTT REQUIRED)
endif()
if(NOT TARGET spdlog::spdlog)
    find_package(spdlog REQUIRED)
endif()
if(NOT TARGET nlohmann_json::nlohmann_json)
    find_package(nlohmann_json REQUIRED)
endif()

# Create shared library
add_library(platformer SHARED
    src/platformer_plugin.cpp
    src/platformer_game_logic.cpp
)

# Include directories
target_include_directories(platformer 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src  # For engine headers
        ${CMAKE_SOURCE_DIR}/src/scripting  # For game logic interface
        ${CMAKE_SOURCE_DIR}/.deps_cache/json-src/include  # For nlohmann/json
)

# Link libraries
target_link_libraries(platformer 
    PRIVATE 
        raylib
        EnTT::EnTT
        spdlog::spdlog
        nlohmann_json::nlohmann_json
)

# Set output name
set_target_properties(platformer PROPERTIES
    OUTPUT_NAME "platformer"
    PREFIX "lib"
)

# Install rules
install(TARGETS platformer
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}
)

install(FILES package.json
    DESTINATION ${CMAKE_INSTALL_PREFIX}
)