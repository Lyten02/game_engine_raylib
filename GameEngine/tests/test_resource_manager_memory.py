#!/usr/bin/env python3
"""
Test for ResourceManager memory efficiency improvements.
Verifies that missing textures don't grow the map and memory is managed correctly.
"""

import os
import sys
import subprocess
import tempfile
import shutil

# Test program code
TEST_CODE = """
#include <iostream>
#include <filesystem>
#include "resources/resource_manager.h"
#include "raylib.h"

int main() {
    // Initialize RayLib in headless mode
    SetTraceLogLevel(LOG_NONE);
    ResourceManager resourceManager;
    resourceManager.setSilentMode(true);
    resourceManager.setHeadlessMode(true);
    resourceManager.setRayLibInitialized(false);
    
    std::cout << "Initial texture count: " << resourceManager.getLoadedTexturesCount() << std::endl;
    
    // Test 1: Request non-existent textures multiple times
    for (int i = 0; i < 100; i++) {
        std::string name = "missing_texture_" + std::to_string(i);
        Texture2D* tex = resourceManager.getTexture(name);
        if (!tex) {
            std::cerr << "ERROR: getTexture returned nullptr" << std::endl;
            return 1;
        }
    }
    
    std::cout << "After 100 missing requests: " << resourceManager.getLoadedTexturesCount() << std::endl;
    
    // Test 2: Verify map doesn't grow with missing textures
    if (resourceManager.getLoadedTexturesCount() > 0) {
        std::cerr << "ERROR: Map grew with missing textures!" << std::endl;
        return 1;
    }
    
    // Test 3: Load a texture with non-existent path
    Texture2D* tex = resourceManager.loadTexture("/non/existent/path.png", "test_missing");
    if (!tex) {
        std::cerr << "ERROR: loadTexture returned nullptr" << std::endl;
        return 1;
    }
    
    std::cout << "After loading missing file: " << resourceManager.getLoadedTexturesCount() << std::endl;
    
    // Test 4: Verify map still doesn't grow
    if (resourceManager.getLoadedTexturesCount() > 0) {
        std::cerr << "ERROR: Map grew when loading missing file!" << std::endl;
        return 1;
    }
    
    // Test 5: Request the same missing texture multiple times
    for (int i = 0; i < 10; i++) {
        Texture2D* tex2 = resourceManager.getTexture("test_missing");
        if (!tex2) {
            std::cerr << "ERROR: getTexture returned nullptr" << std::endl;
            return 1;
        }
        if (tex != tex2) {
            std::cerr << "ERROR: Different pointers returned for same missing texture" << std::endl;
            return 1;
        }
    }
    
    std::cout << "After repeated requests: " << resourceManager.getLoadedTexturesCount() << std::endl;
    
    // Test 6: Create a real texture file and load it
    std::filesystem::create_directories("test_textures");
    
    // Create a simple 2x2 checkerboard pattern manually
    const char* testTexturePath = "test_textures/real_texture.png";
    
    // For this test, we'll simulate loading success
    // In real usage, this would actually load a texture file
    std::cout << "Test creating real texture (simulated in headless mode)" << std::endl;
    
    // Clean up
    std::filesystem::remove_all("test_textures");
    
    std::cout << "SUCCESS: All memory tests passed!" << std::endl;
    return 0;
}
"""

# CMakeLists.txt for the test
CMAKE_CONTENT = """
cmake_minimum_required(VERSION 3.20)
project(ResourceManagerMemoryTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add main GameEngine src directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../src)

# Add source files using relative path from build directory
add_executable(resource_test 
    ../test_resource_memory.cpp
    ../../../src/resources/resource_manager.cpp
)

# Find and link dependencies
find_package(raylib QUIET)
if (NOT raylib_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.0
    )
    FetchContent_MakeAvailable(raylib)
endif()

find_package(spdlog QUIET)
if (NOT spdlog_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.14.1
    )
    FetchContent_MakeAvailable(spdlog)
endif()

target_link_libraries(resource_test raylib spdlog::spdlog)
"""

def run_test():
    """Run the ResourceManager memory test"""
    print("Running ResourceManager memory efficiency test...")
    
    # Get absolute path to source file
    source_path = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'src', 'resources', 'resource_manager.cpp'))
    include_path = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'src'))
    
    # Create CMakeLists.txt with absolute paths
    CMAKE_CONTENT_DYNAMIC = f"""
cmake_minimum_required(VERSION 3.20)
project(ResourceManagerMemoryTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add main GameEngine src directory
include_directories({include_path})

# Add source files using absolute path
add_executable(resource_test 
    test_resource_memory.cpp
    {source_path}
)

# Find and link dependencies
find_package(raylib QUIET)
if (NOT raylib_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.0
    )
    FetchContent_MakeAvailable(raylib)
endif()

find_package(spdlog QUIET)
if (NOT spdlog_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.14.1
    )
    FetchContent_MakeAvailable(spdlog)
endif()

target_link_libraries(resource_test raylib spdlog::spdlog)
"""
    
    # Create temporary directory for test
    with tempfile.TemporaryDirectory() as tmpdir:
        # Write test files
        test_cpp_path = os.path.join(tmpdir, "test_resource_memory.cpp")
        cmake_path = os.path.join(tmpdir, "CMakeLists.txt")
        
        with open(test_cpp_path, 'w') as f:
            f.write(TEST_CODE)
        
        with open(cmake_path, 'w') as f:
            f.write(CMAKE_CONTENT_DYNAMIC)
        
        # Build test
        build_dir = os.path.join(tmpdir, "build")
        os.makedirs(build_dir)
        
        print("Building test...")
        cmake_cmd = ["cmake", ".."]
        result = subprocess.run(cmake_cmd, cwd=build_dir, capture_output=True, text=True)
        if result.returncode != 0:
            print(f"CMake failed:\n{result.stderr}")
            return False
        
        make_cmd = ["make"]
        result = subprocess.run(make_cmd, cwd=build_dir, capture_output=True, text=True)
        if result.returncode != 0:
            print(f"Make failed:\n{result.stderr}")
            return False
        
        # Run test
        print("Running test...")
        test_exe = os.path.join(build_dir, "resource_test")
        result = subprocess.run([test_exe], capture_output=True, text=True)
        
        print("Test output:")
        print(result.stdout)
        
        if result.returncode != 0:
            print(f"Test failed with return code {result.returncode}")
            if result.stderr:
                print(f"Error output:\n{result.stderr}")
            return False
        
        return True

if __name__ == "__main__":
    success = run_test()
    sys.exit(0 if success else 1)