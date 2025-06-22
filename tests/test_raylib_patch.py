#!/usr/bin/env python3
"""Test raylib CMake patch functionality."""

import os
import sys
from pathlib import Path

def run_tests():
    """Run all tests for raylib patch."""
    failed_tests = []
    passed_tests = []
    
    patches_dir = Path("GameEngine/patches")
    raylib_patch = patches_dir / "raylib-cmake-fix.patch"
    
    # Test 1: Patches directory exists
    try:
        assert patches_dir.exists(), "Patches directory should exist"
        assert patches_dir.is_dir(), "Patches should be a directory"
        passed_tests.append("test_patches_dir_exists")
    except AssertionError as e:
        failed_tests.append(("test_patches_dir_exists", str(e)))
    
    # Test 2: Raylib patch file exists
    try:
        assert raylib_patch.exists(), "Raylib CMake patch file should exist"
        assert raylib_patch.is_file(), "Patch should be a file"
        passed_tests.append("test_raylib_patch_exists")
    except AssertionError as e:
        failed_tests.append(("test_raylib_patch_exists", str(e)))
    
    # Test 3: Patch content is valid
    try:
        if raylib_patch.exists():
            with open(raylib_patch, 'r') as f:
                content = f.read()
                
                # Check for patch header
                assert "diff" in content or "---" in content, \
                    "Patch should contain diff markers"
                
                # Check for CMake version update
                assert "cmake_minimum_required" in content, \
                    "Patch should update cmake_minimum_required"
                assert "3.5" in content and ("3.11" in content or "3.15" in content), \
                    "Patch should update from 3.5 to newer version"
                
                passed_tests.append("test_patch_content_valid")
        else:
            failed_tests.append(("test_patch_content_valid", "Patch file doesn't exist"))
    except AssertionError as e:
        failed_tests.append(("test_patch_content_valid", str(e)))
    
    # Test 4: CMakeLists.txt handles raylib CMake version
    try:
        cmake_file = Path("GameEngine/CMakeLists.txt")
        if cmake_file.exists():
            with open(cmake_file, 'r') as f:
                content = f.read()
                
                assert "string(REGEX REPLACE" in content or "PATCH_COMMAND" in content, \
                    "CMakeLists.txt should handle raylib CMake version"
                
                passed_tests.append("test_cmake_handles_version")
        else:
            failed_tests.append(("test_cmake_handles_version", "CMakeLists.txt doesn't exist"))
    except AssertionError as e:
        failed_tests.append(("test_cmake_handles_version", str(e)))
    
    # Print results
    print(f"\n{'='*60}")
    print("Raylib Patch Tests")
    print(f"{'='*60}\n")
    
    if passed_tests:
        print(f"✅ Passed ({len(passed_tests)}):")
        for test in passed_tests:
            print(f"   - {test}")
    
    if failed_tests:
        print(f"\n❌ Failed ({len(failed_tests)}):")
        for test_name, error in failed_tests:
            print(f"   - {test_name}: {error}")
    
    print(f"\n{'='*60}")
    print(f"Total: {len(passed_tests)} passed, {len(failed_tests)} failed")
    print(f"{'='*60}\n")
    
    return len(failed_tests)

if __name__ == "__main__":
    sys.exit(run_tests())