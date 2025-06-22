#!/usr/bin/env python3

import subprocess
import sys
import os
from pathlib import Path
import json

def test_cmake_cache_exists():
    """Test if CMake cache exists after configuration"""
    build_dir = Path(__file__).parent.parent / "build"
    cmake_cache = build_dir / "CMakeCache.txt"
    
    print("=== Testing CMake Cache ===")
    
    if not build_dir.exists():
        print("❌ Build directory doesn't exist")
        return False
    
    if cmake_cache.exists():
        print("✅ CMakeCache.txt exists")
        
        # Check for important variables
        with open(cmake_cache, 'r') as f:
            content = f.read()
            
        important_vars = [
            'CMAKE_CXX_COMPILER',
            'CMAKE_C_COMPILER',
            'CMAKE_BUILD_TYPE',
            'Python3_EXECUTABLE'
        ]
        
        for var in important_vars:
            if f'{var}:' in content:
                line = [l for l in content.split('\n') if f'{var}:' in l][0]
                print(f"  ✓ {var} = {line.split('=')[1] if '=' in line else 'set'}")
            else:
                print(f"  ❌ {var} not found")
        
        return True
    else:
        print("❌ CMakeCache.txt not found")
        return False

def test_dependencies_downloaded():
    """Test if dependencies are properly downloaded"""
    build_dir = Path(__file__).parent.parent / "build"
    deps_cache = Path(__file__).parent.parent / ".deps_cache"
    
    print("\n=== Testing Dependencies ===")
    
    # Check global deps cache
    if deps_cache.exists():
        print(f"✅ Global deps cache exists: {deps_cache}")
        
        # Check for specific dependencies
        expected_deps = ['raylib', 'spdlog', 'entt', 'glm', 'json']
        for dep in expected_deps:
            dep_dir = deps_cache / f"{dep}-src"
            if dep_dir.exists():
                print(f"  ✓ {dep} downloaded")
            else:
                print(f"  ❌ {dep} not found")
    else:
        print("❌ Global deps cache not found")
        
    # Check build directory deps
    build_deps = build_dir / "_deps"
    if build_deps.exists():
        print(f"✅ Build deps directory exists: {build_deps}")
    
    return True

def test_executable_built():
    """Test if game_engine executable was built"""
    build_dir = Path(__file__).parent.parent / "build"
    
    print("\n=== Testing Executable ===")
    
    # Check for executable based on platform
    if sys.platform == 'win32':
        exe_name = "game_engine.exe"
    else:
        exe_name = "game_engine"
    
    exe_path = build_dir / exe_name
    
    if exe_path.exists():
        print(f"✅ {exe_name} exists")
        
        # Check size
        size = exe_path.stat().st_size
        print(f"  Size: {size:,} bytes")
        
        if size < 1000:
            print("  ⚠️  Executable seems too small")
            return False
        
        # Check if executable on Unix
        if sys.platform != 'win32':
            if os.access(exe_path, os.X_OK):
                print("  ✓ File is executable")
            else:
                print("  ❌ File is not executable")
                return False
        
        return True
    else:
        print(f"❌ {exe_name} not found")
        
        # List what's in build directory
        if build_dir.exists():
            print("\nFiles in build directory:")
            files = list(build_dir.glob("*"))[:20]
            for f in files:
                print(f"  - {f.name}")
        
        return False

def test_test_discovery_works():
    """Test if test discovery finds tests correctly"""
    tests_dir = Path(__file__).parent
    cmake_module = Path(__file__).parent.parent / "cmake" / "TestDiscovery.cmake"
    
    print("\n=== Testing Test Discovery ===")
    
    if not cmake_module.exists():
        print("❌ TestDiscovery.cmake not found")
        return False
    
    print("✅ TestDiscovery.cmake exists")
    
    # Count test files
    py_tests = list(tests_dir.glob("**/test_*.py"))
    cpp_tests = list(tests_dir.glob("**/test_*.cpp"))
    txt_tests = list(tests_dir.glob("**/*test*.txt"))
    
    print(f"  Python tests found: {len(py_tests)}")
    print(f"  C++ tests found: {len(cpp_tests)}")
    print(f"  Script tests found: {len(txt_tests)}")
    
    if len(py_tests) == 0:
        print("  ⚠️  No Python tests found")
    
    return True

if __name__ == "__main__":
    tests = [
        test_cmake_cache_exists,
        test_dependencies_downloaded,
        test_executable_built,
        test_test_discovery_works
    ]
    
    all_passed = True
    for test in tests:
        if not test():
            all_passed = False
    
    print("\n" + "="*50)
    if all_passed:
        print("✅ All build checks passed")
    else:
        print("❌ Some build checks failed")
    
    sys.exit(0 if all_passed else 1)