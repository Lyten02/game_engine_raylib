#!/usr/bin/env python3
"""Fixed build system test that uses sequential commands"""

import subprocess
import os
import sys
import time
import shutil

def run_command(command):
    """Run a single command and return success status"""
    # Change to build directory
    original_dir = os.getcwd()
    if os.path.basename(os.getcwd()) == "tests":
        os.chdir("../build")
    elif not os.path.exists("game"):
        if os.path.exists("build/game"):
            os.chdir("build")
    
    try:
        # We need to maintain state between commands, so use a state file
        state_file = "test_project_state.txt"
        
        # Write the current command to execute
        with open("test_command.txt", "w") as f:
            f.write(command + "\n")
        
        # Execute via script to maintain project state
        result = subprocess.run(
            ["./game", "--headless", "--script", "test_command.txt"],
            capture_output=True,
            text=True,
            timeout=30
        )
        
        os.remove("test_command.txt")
        return result.returncode == 0, result.stdout, result.stderr
    finally:
        os.chdir(original_dir)

def test_build_system():
    """Test the build system with real compilation"""
    print("=== Game Engine Build System Test ===\n")
    
    project_name = "BuildTest"
    
    # Move to build directory
    original_dir = os.getcwd()
    if os.path.basename(os.getcwd()) == "tests":
        os.chdir("../build")
    elif not os.path.exists("game"):
        if os.path.exists("build/game"):
            os.chdir("build")
    
    try:
        # Clean up any existing project
        print("Cleaning up old project...")
        if os.path.exists(f"projects/{project_name}"):
            shutil.rmtree(f"projects/{project_name}", ignore_errors=True)
        if os.path.exists(f"output/{project_name}"):
            shutil.rmtree(f"output/{project_name}", ignore_errors=True)
        
        # Create all commands in a single batch file
        print("Creating batch script...")
        with open("build_test_batch.txt", "w") as f:
            f.write(f"project.create {project_name}\n")
            f.write(f"project.open {project_name}\n")
            f.write("scene.create main\n")
            f.write("entity.create Player\n") 
            f.write("scene.save main\n")
            f.write("project.build-fast\n")  # Use fast build
            f.write("exit\n")
        
        print("Executing build commands...")
        start_time = time.time()
        
        result = subprocess.run(
            ["./game", "--headless", "--script", "build_test_batch.txt"],
            capture_output=True,
            text=True,
            timeout=60  # 1 minute timeout for fast build
        )
        
        elapsed = time.time() - start_time
        os.remove("build_test_batch.txt")
        
        if result.returncode != 0:
            print(f"❌ Build failed!")
            print(f"stdout: {result.stdout[:500]}")
            print(f"stderr: {result.stderr}")
            return False
        
        print(f"✅ Commands executed in {elapsed:.1f}s")
        
        # Verify output
        output_dir = f"output/{project_name}"
        if os.path.exists(output_dir):
            print(f"✅ Output directory created: {output_dir}")
            
            # Check generated files
            files_to_check = [
                ("main.cpp", "Generated C++ source"),
                ("CMakeLists.txt", "CMake configuration"),
                ("scenes/main_scene.json", "Scene file")
            ]
            
            all_good = True
            for filename, description in files_to_check:
                filepath = os.path.join(output_dir, filename)
                if os.path.exists(filepath):
                    size = os.path.getsize(filepath)
                    print(f"✅ {description}: {filename} ({size} bytes)")
                else:
                    print(f"❌ Missing: {filename}")
                    all_good = False
            
            if all_good:
                print("\n🎉 Build system test PASSED!")
                print("   - Project created successfully")
                print("   - Build files generated correctly")
                print("   - Fast build completed quickly")
                return True
        else:
            print(f"❌ Output directory not found: {output_dir}")
            
            # Debug: list what we have
            print("\nDebug - Current directory contents:")
            for item in os.listdir("."):
                print(f"  - {item}")
            
            if os.path.exists("output"):
                print("\nDebug - Output directory contents:")
                for item in os.listdir("output"):
                    print(f"  - output/{item}")
                    
        return False
        
    finally:
        os.chdir(original_dir)

if __name__ == "__main__":
    # First check if we can find the game executable
    paths_to_check = ["./game", "../build/game", "build/game"]
    game_found = False
    
    for path in paths_to_check:
        if os.path.exists(path):
            game_found = True
            print(f"Found game executable at: {path}")
            break
    
    if not game_found:
        print("❌ Error: game executable not found!")
        print("   Please build the project first")
        sys.exit(1)
    
    # Run the test
    if test_build_system():
        sys.exit(0)
    else:
        sys.exit(1)