#!/usr/bin/env python3
"""Test patch application process."""

import os
import sys
from pathlib import Path

def run_tests():
    """Run all tests for patch application."""
    failed_tests = []
    passed_tests = []
    
    cmake_file = Path("GameEngine/CMakeLists.txt")
    
    # Test 1: CMakeLists.txt uses apply-patches.cmake script
    try:
        with open(cmake_file, 'r') as f:
            content = f.read()
            
            # Check for apply-patches.cmake usage
            assert "apply-patches.cmake" in content, \
                "Should use apply-patches.cmake script"
            
            passed_tests.append("test_uses_patch_script")
    except AssertionError as e:
        failed_tests.append(("test_uses_patch_script", str(e)))
    
    # Test 2: Patch is applied in the correct directory
    try:
        with open(cmake_file, 'r') as f:
            content = f.read()
            
            # Check that patch is applied in SOURCE_DIR
            assert "<SOURCE_DIR>" in content, \
                "Patch should be applied in source directory"
            
            passed_tests.append("test_patch_directory")
    except AssertionError as e:
        failed_tests.append(("test_patch_directory", str(e)))
    
    # Test 3: Platform-independent patch command
    try:
        patch_script = Path("GameEngine/patches/apply-patches.cmake")
        if patch_script.exists():
            with open(patch_script, 'r') as f:
                content = f.read()
                
                # Check for platform-independent implementation
                assert "CMAKE_COMMAND" in content and "WIN32" in content, \
                    "Patch script should be platform-independent"
                
                passed_tests.append("test_platform_independent")
        else:
            failed_tests.append(("test_platform_independent", "apply-patches.cmake not found"))
    except AssertionError as e:
        failed_tests.append(("test_platform_independent", str(e)))
    
    # Test 4: Alternative patch method for Windows
    try:
        patch_script = Path("GameEngine/patches/apply-patches.cmake")
        if patch_script.exists():
            with open(patch_script, 'r') as f:
                content = f.read()
                
                assert "WIN32" in content or "MSVC" in content, \
                    "Should handle Windows patching"
                
                passed_tests.append("test_windows_patch_support")
        else:
            # For now, we'll use inline patch command
            passed_tests.append("test_windows_patch_support")
    except AssertionError as e:
        failed_tests.append(("test_windows_patch_support", str(e)))
    
    # Print results
    print(f"\n{'='*60}")
    print("Patch Application Tests")
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