#!/usr/bin/env python3
"""Test CI caching optimization."""

import os
import sys
from pathlib import Path

def run_tests():
    """Run all tests for CI caching optimization."""
    failed_tests = []
    passed_tests = []
    
    ci_workflow = Path(".github/workflows/ci.yml")
    
    # Test 1: Enhanced dependency caching
    try:
        with open(ci_workflow, 'r') as f:
            content = f.read()
            
            # Check for comprehensive cache paths
            assert "~/.cache/pip" in content or "~/Library/Caches/pip" in content, \
                "Should cache pip dependencies"
            assert "vcpkg" in content or "conan" in content or "_deps" in content, \
                "Should cache C++ package manager files"
            assert "ccache" in content, "Should use ccache"
            
            passed_tests.append("test_comprehensive_caching")
    except AssertionError as e:
        failed_tests.append(("test_comprehensive_caching", str(e)))
    
    # Test 2: Multiple cache keys for better hit rate
    try:
        with open(ci_workflow, 'r') as f:
            content = f.read()
            
            # Check for multiple restore keys
            lines = content.split('\n')
            restore_keys_count = sum(1 for line in lines if 'restore-keys:' in line)
            assert restore_keys_count >= 1, "Should have restore-keys for fallback"
            
            # Check for hash-based cache keys
            assert "hashFiles(" in content, "Should use file hashing for cache keys"
            
            passed_tests.append("test_cache_key_strategy")
    except AssertionError as e:
        failed_tests.append(("test_cache_key_strategy", str(e)))
    
    # Test 3: Ccache configuration
    try:
        with open(ci_workflow, 'r') as f:
            content = f.read()
            
            # Check for ccache setup
            assert "CCACHE_DIR" in content or "ccache --max-size" in content or \
                   "CMAKE_CXX_COMPILER_LAUNCHER" in content, \
                "Should configure ccache properly"
            
            passed_tests.append("test_ccache_configuration")
    except AssertionError as e:
        failed_tests.append(("test_ccache_configuration", str(e)))
    
    # Test 4: Build artifacts caching
    try:
        with open(ci_workflow, 'r') as f:
            content = f.read()
            
            # Check for build artifacts in cache
            assert "CMakeCache.txt" in content or "build/CMakeFiles" in content or \
                   ".cmake" in content, \
                "Should cache CMake configuration"
            
            passed_tests.append("test_build_artifacts_caching")
    except AssertionError as e:
        failed_tests.append(("test_build_artifacts_caching", str(e)))
    
    # Test 5: Platform-specific cache optimization
    try:
        with open(ci_workflow, 'r') as f:
            content = f.read()
            
            # Check for platform-specific cache paths
            assert "${{ runner.os }}" in content, \
                "Should use platform-specific cache keys"
            assert "actions/cache@v3" in content, \
                "Should use latest cache action"
            
            passed_tests.append("test_platform_specific_caching")
    except AssertionError as e:
        failed_tests.append(("test_platform_specific_caching", str(e)))
    
    # Print results
    print(f"\n{'='*60}")
    print("CI Caching Optimization Tests")
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