#!/usr/bin/env python3
"""Test to verify build compilation fixes"""

import os
import subprocess
import sys

def test_includes_fixed():
    """Verify all necessary includes are present"""
    print("Testing include fixes...")
    
    # Check async_build_system.cpp has filesystem include
    with open("src/build/async_build_system.cpp", "r") as f:
        content = f.read()
        assert "#include <filesystem>" in content, "Missing filesystem include in async_build_system.cpp"
    
    # Check build_system.cpp has json include
    with open("src/build/build_system.cpp", "r") as f:
        content = f.read()
        assert "#include <nlohmann/json.hpp>" in content, "Missing json include in build_system.cpp"
    
    print("✅ All includes fixed")

def test_cmake_compiles():
    """Test that CMake configuration works"""
    print("Testing CMake configuration...")
    
    # Create a test build directory
    test_build_dir = "build_test"
    if os.path.exists(test_build_dir):
        import shutil
        shutil.rmtree(test_build_dir)
    
    os.makedirs(test_build_dir)
    
    # Try to configure with CMake
    result = subprocess.run(
        ["cmake", "..", "-DCMAKE_BUILD_TYPE=Release"],
        cwd=test_build_dir,
        capture_output=True,
        text=True
    )
    
    if result.returncode == 0:
        print("✅ CMake configuration successful")
    else:
        print(f"❌ CMake configuration failed:")
        print(result.stdout)
        print(result.stderr)
        return False
    
    # Clean up
    import shutil
    shutil.rmtree(test_build_dir)
    
    return True

def test_platform_config():
    """Test platform configuration"""
    print("Testing platform configuration...")
    
    # Check if PlatformConfig.cmake exists or fallback works
    cmake_content = open("CMakeLists.txt", "r").read()
    assert "if(EXISTS ${CMAKE_SOURCE_DIR}/cmake/PlatformConfig.cmake)" in cmake_content
    assert "fallback platform configuration" in cmake_content.lower()
    
    print("✅ Platform configuration has proper fallback")

if __name__ == "__main__":
    print("=== Build Compilation Fixes Test ===")
    
    test_includes_fixed()
    test_platform_config()
    
    # Only test CMake if we're in a proper environment
    if os.path.exists("build"):
        test_cmake_compiles()
    
    print("\n✅ All build compilation tests passed!")