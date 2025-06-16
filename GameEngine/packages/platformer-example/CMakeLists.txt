cmake_minimum_required(VERSION 3.20)
project(platformer-example VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(raylib REQUIRED)
find_package(EnTT REQUIRED)
find_package(spdlog REQUIRED)

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
)

# Link libraries
target_link_libraries(platformer 
    PRIVATE 
        raylib
        EnTT::EnTT
        spdlog::spdlog
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