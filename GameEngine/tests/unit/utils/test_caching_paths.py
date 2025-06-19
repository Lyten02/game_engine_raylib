#!/usr/bin/env python3
"""Test that caching paths are correct in build scripts"""

import os
import sys
import subprocess

def test_caching_path_in_rebuild_smart():
    """Test that rebuild_smart.sh checks the correct cache directory"""
    print("Testing cache path in rebuild_smart.sh...")
    
    with open('../rebuild_smart.sh', 'r') as f:
        content = f.read()
    
    # Check if it's checking for the correct cache directory
    if '.deps_cache' in content and 'build/_deps' not in content:
        print("✓ rebuild_smart.sh checks '.deps_cache' directory")
        return True
    elif 'build/_deps' in content:
        print("✗ rebuild_smart.sh incorrectly checks 'build/_deps' instead of '.deps_cache'")
        return False
    else:
        print("✗ rebuild_smart.sh doesn't check cache directory properly")
        return False

def test_cmake_cache_configuration():
    """Test that CMakeLists.txt configures cache correctly"""
    print("\nTesting CMake cache configuration...")
    
    with open('../CMakeLists.txt', 'r') as f:
        content = f.read()
    
    if 'GLOBAL_DEPS_CACHE' in content and '.deps_cache' in content:
        print("✓ CMakeLists.txt correctly configures .deps_cache")
        return True
    else:
        print("✗ CMakeLists.txt doesn't configure cache correctly")
        return False

def test_cache_directory_exists():
    """Test that the cache directory actually exists after a build"""
    print("\nTesting cache directory existence...")
    
    if os.path.exists('../.deps_cache'):
        print("✓ .deps_cache directory exists")
        return True
    else:
        print("✗ .deps_cache directory does not exist")
        return False

def main():
    print("=== Caching Path Tests ===\n")
    
    tests = [
        test_caching_path_in_rebuild_smart,
        test_cmake_cache_configuration,
        test_cache_directory_exists
    ]
    
    passed = sum(test() for test in tests)
    total = len(tests)
    
    print(f"\n=== Results: {passed}/{total} tests passed ===")
    
    return 0 if passed == total else 1

if __name__ == "__main__":
    sys.exit(main())