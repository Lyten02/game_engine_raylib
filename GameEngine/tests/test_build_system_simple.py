#!/usr/bin/env python3
"""Simple test for build system"""

import subprocess
import os
import time
import sys

def test_build_fast():
    """Test fast build functionality"""
    print("Testing fast build system...")
    
    # Change to build directory
    os.chdir(os.path.join(os.path.dirname(__file__), '../build'))
    
    project_name = "BuildTestFast"
    
    # Clean up any existing project
    subprocess.run(["./game_engine", "--headless", "--command", f"project.delete {project_name}"], 
                   capture_output=True, timeout=10)
    
    # Create project
    print("1. Creating project...")
    result = subprocess.run(["./game_engine", "--headless", "--command", f"project.create {project_name}"], 
                          capture_output=True, text=True, timeout=10)
    if result.returncode != 0:
        print(f"Failed to create project: {result.stderr}")
        return False
    
    # Open project
    print("2. Opening project...")
    result = subprocess.run(["./game_engine", "--headless", "--command", f"project.open {project_name}"], 
                          capture_output=True, text=True, timeout=10)
    if result.returncode != 0:
        print(f"Failed to open project: {result.stderr}")
        return False
    
    # Create scene
    print("3. Creating scene...")
    result = subprocess.run(["./game_engine", "--headless", "--command", "scene.create main"], 
                          capture_output=True, text=True, timeout=10)
    if result.returncode != 0:
        print(f"Failed to create scene: {result.stderr}")
        return False
    
    # Create entity
    print("4. Creating entity...")
    result = subprocess.run(["./game_engine", "--headless", "--command", "entity.create Player"], 
                          capture_output=True, text=True, timeout=10)
    if result.returncode != 0:
        print(f"Failed to create entity: {result.stderr}")
        return False
    
    # Save scene
    print("5. Saving scene...")
    result = subprocess.run(["./game_engine", "--headless", "--command", "scene.save main"], 
                          capture_output=True, text=True, timeout=10)
    if result.returncode != 0:
        print(f"Failed to save scene: {result.stderr}")
        return False
    
    # Fast build
    print("6. Running fast build...")
    start_time = time.time()
    result = subprocess.run(["./game_engine", "--headless", "--command", "project.build-fast"], 
                          capture_output=True, text=True, timeout=30)
    elapsed = time.time() - start_time
    
    if result.returncode != 0:
        print(f"Failed to build: {result.stderr}")
        return False
    
    print(f"✅ Fast build completed in {elapsed:.1f}s")
    
    # Check output files
    output_dir = f"output/{project_name}"
    if os.path.exists(output_dir):
        print(f"✅ Output directory created: {output_dir}")
        
        # Check for generated files
        main_cpp = os.path.join(output_dir, "main.cpp")
        cmake_lists = os.path.join(output_dir, "CMakeLists.txt")
        
        if os.path.exists(main_cpp):
            print("✅ Generated main.cpp")
        else:
            print("❌ main.cpp not found")
            return False
            
        if os.path.exists(cmake_lists):
            print("✅ Generated CMakeLists.txt")
        else:
            print("❌ CMakeLists.txt not found")
            return False
    else:
        print(f"❌ Output directory not found: {output_dir}")
        return False
    
    return True

if __name__ == "__main__":
    print("=== Simple Build System Test ===\n")
    
    if test_build_fast():
        print("\n🎉 Test passed!")
        sys.exit(0)
    else:
        print("\n❌ Test failed!")
        sys.exit(1)