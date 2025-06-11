#!/usr/bin/env python3
"""Test that fast build is significantly faster than full build"""

import subprocess
import time
import os
import shutil
import sys
from cleanup_utils import TestProjectCleaner

def run_command(cmd, capture_output=True):
    """Run a command and return result"""
    if isinstance(cmd, str):
        cmd = cmd.split()
    
    result = subprocess.run(cmd, capture_output=capture_output, text=True)
    return result

def test_fast_build_performance():
    """Test that fast build is significantly faster than full build"""
    
    print("Testing fast build performance...")
    
    # Tests are always run from build directory
    game_exe = "./game"
    projects_dir = "../projects"
    output_dir = "../output"
    test_project = "FastBuildTest"
    
    # Use context manager for automatic cleanup
    with TestProjectCleaner(test_project, preserve_cache=True):
        # Create test script for full build
        full_build_script = f"""project.create {test_project}
project.open {test_project}
scene.create main
entity.create Player
project.build
"""
        
        with open("test_full_build.txt", "w") as f:
            f.write(full_build_script)
        
        # First build (should be slow, builds dependencies)
        print("Running full build (this will take a while)...")
        start = time.time()
        result = run_command([game_exe, "--script", "test_full_build.txt"])
        full_build_time = time.time() - start
        
        if result.returncode != 0:
            print(f"Full build failed: {result.stderr}")
            print(f"Output: {result.stdout}")
            os.remove("test_full_build.txt")
            return False
        
        print(f"Full build completed in {full_build_time:.1f} seconds")
        os.remove("test_full_build.txt")
        
        # Clean build directory but keep _deps
        build_dir = f"{output_dir}/{test_project}/build"
        if os.path.exists(build_dir):
            for item in os.listdir(build_dir):
                if item != "_deps":
                    path = os.path.join(build_dir, item)
                    if os.path.isfile(path):
                        os.remove(path)
                    elif os.path.isdir(path):
                        shutil.rmtree(path)
        
        # Also remove the output executable to force rebuild
        project_output_dir = f"{output_dir}/{test_project}"
        if os.path.exists(f"{project_output_dir}/game"):
            os.remove(f"{project_output_dir}/game")
        
        # Create test script for fast build
        fast_build_script = f"""project.open {test_project}
project.build.fast
"""
        
        with open("test_fast_build.txt", "w") as f:
            f.write(fast_build_script)
        
        # Fast build (should be much faster)
        print("Running fast build...")
        start = time.time()
        result = run_command([game_exe, "--script", "test_fast_build.txt"])
        fast_build_time = time.time() - start
        
        if result.returncode != 0:
            print(f"Fast build failed: {result.stderr}")
            print(f"Output: {result.stdout}")
            os.remove("test_fast_build.txt")
            return False
        
        os.remove("test_fast_build.txt")
        
        print(f"Fast build completed in {fast_build_time:.1f} seconds")
        
        # Calculate speedup
        speedup = 1.0
        if fast_build_time > 0:
            speedup = full_build_time / fast_build_time
            print(f"Speedup: {speedup:.1f}x")
            
            # Verify fast build is at least 3x faster
            if speedup < 3.0:
                print(f"WARNING: Fast build not fast enough (only {speedup:.1f}x speedup)")
                print(f"Expected at least 3x speedup")
            
            # Verify fast build is under 30 seconds
            if fast_build_time > 30:
                print(f"WARNING: Fast build too slow ({fast_build_time:.1f}s)")
                print(f"Expected under 30 seconds")
        
        # Verify build files were created (fast build only generates source files)
        build_files = [
            f"{output_dir}/{test_project}/main.cpp",
            f"{output_dir}/{test_project}/CMakeLists.txt",
            f"{output_dir}/{test_project}/game_config.json"
        ]
        
        all_files_exist = True
        for file_path in build_files:
            if os.path.exists(file_path):
                print(f"✓ Build file created: {os.path.basename(file_path)}")
            else:
                print(f"ERROR: Build file not found: {file_path}")
                all_files_exist = False
        
        if not all_files_exist:
            return False
        
        # Summary
        print("\n=== Summary ===")
        print(f"Full build time: {full_build_time:.1f}s")
        print(f"Fast build time: {fast_build_time:.1f}s") 
        print(f"Speedup: {speedup:.1f}x")
        
        success = speedup >= 3.0 and fast_build_time < 30
        if success:
            print("✓ Fast build test PASSED")
        else:
            print("✗ Fast build test FAILED")
        
        return success

if __name__ == "__main__":
    # Tests are run from build directory
    deps_path = "_deps/raylib-build/raylib/libraylib.a"
    cache_path = "../.deps_cache/_deps/raylib-build/raylib/libraylib.a"
    
    # Ensure main project has been built first
    if not os.path.exists(deps_path) and not os.path.exists(cache_path):
        print("ERROR: Main project dependencies not found.")
        print("Please run 'make' in the build directory first.")
        sys.exit(1)
    
    success = test_fast_build_performance()
    sys.exit(0 if success else 1)