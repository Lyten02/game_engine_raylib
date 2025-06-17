#!/usr/bin/env python3
"""
Test rebuild caching functionality
"""

import os
import sys
import subprocess
import time
import shutil
import json

# Add parent directory to path
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

def run_command(cmd, cwd=None):
    """Run command and return output"""
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True, cwd=cwd)
    return result.returncode, result.stdout, result.stderr

def test_dependency_caching():
    """Test that dependencies are cached properly between builds"""
    print("Testing dependency caching...")
    
    # Get to GameEngine directory
    game_engine_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    os.chdir(game_engine_dir)
    
    # Clean everything first
    print("1. Cleaning all build artifacts and cache...")
    if os.path.exists("build"):
        shutil.rmtree("build")
    if os.path.exists(".deps_cache"):
        shutil.rmtree(".deps_cache")
    
    # First build - should download dependencies
    print("2. First build (should download dependencies)...")
    start_time = time.time()
    returncode, stdout, stderr = run_command("./rebuild_fast.sh")
    first_build_time = time.time() - start_time
    
    assert returncode == 0, f"First build failed: {stderr}"
    assert "Populating raylib" in stdout, "Should download raylib on first build"
    assert "Populating spdlog" in stdout, "Should download spdlog on first build"
    assert os.path.exists(".deps_cache"), "Dependency cache should be created"
    
    print(f"   First build time: {first_build_time:.2f}s")
    
    # Second build - should use cached dependencies
    print("3. Second build (should use cached dependencies)...")
    start_time = time.time()
    returncode, stdout, stderr = run_command("./rebuild_fast.sh")
    second_build_time = time.time() - start_time
    
    assert returncode == 0, f"Second build failed: {stderr}"
    # When cache is used, CMake still shows "Populating" but it's instant
    # The real indicator is the build time difference
    assert "Found cached dependencies" in stdout, "Should report finding cached dependencies"
    assert os.path.exists(".deps_cache"), "Dependency cache should still exist"
    
    print(f"   Second build time: {second_build_time:.2f}s")
    
    # Second build should be significantly faster
    assert second_build_time < first_build_time * 0.5, \
        f"Second build ({second_build_time:.2f}s) should be much faster than first ({first_build_time:.2f}s)"
    
    print("✅ Dependency caching test passed!")
    
def test_smart_rebuild_cache_detection():
    """Test that smart rebuild correctly detects cache"""
    print("\nTesting smart rebuild cache detection...")
    
    game_engine_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    os.chdir(game_engine_dir)
    
    # Ensure cache exists
    if not os.path.exists(".deps_cache"):
        run_command("./rebuild_fast.sh")
    
    # Touch a source file to trigger rebuild
    run_command("touch src/main.cpp")
    
    # Run smart rebuild
    returncode, stdout, stderr = run_command("./rebuild_smart.sh")
    
    assert returncode == 0, f"Smart rebuild failed: {stderr}"
    assert "Running fast rebuild" in stdout or "Running incremental build" in stdout, \
        "Smart rebuild should use fast/incremental when cache exists"
    assert "Populating raylib" not in stdout, "Should not re-download dependencies"
    
    print("✅ Smart rebuild cache detection test passed!")

def main():
    """Run all tests"""
    print("=== Testing Rebuild Caching ===")
    
    try:
        test_dependency_caching()
        test_smart_rebuild_cache_detection()
        print("\n✅ All caching tests passed!")
        return 0
    except AssertionError as e:
        print(f"\n❌ Test failed: {e}")
        return 1
    except Exception as e:
        print(f"\n❌ Unexpected error: {e}")
        return 1

if __name__ == "__main__":
    sys.exit(main())