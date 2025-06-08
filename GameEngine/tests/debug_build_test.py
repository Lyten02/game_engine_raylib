#!/usr/bin/env python3
"""Debug version of build test to see full output"""

import subprocess
import os
import sys
import shutil

def test_full_build():
    """Test full build with complete output"""
    print("=== Debug Build Test ===\n")
    
    # Move to build directory
    original_dir = os.getcwd()
    if os.path.basename(os.getcwd()) == "tests":
        os.chdir("../build")
    elif not os.path.exists("game"):
        if os.path.exists("build/game"):
            os.chdir("build")
    
    try:
        project_name = "BuildTestDebug"
        
        # Clean up
        if os.path.exists(f"projects/{project_name}"):
            shutil.rmtree(f"projects/{project_name}", ignore_errors=True)
        if os.path.exists(f"output/{project_name}"):
            shutil.rmtree(f"output/{project_name}", ignore_errors=True)
        
        # Create script
        script_name = "debug_build_test.txt"
        with open(script_name, "w") as f:
            f.write(f"project.create {project_name}\n")
            f.write(f"project.open {project_name}\n")
            f.write("scene.create main\n")
            f.write("entity.create Player\n") 
            f.write("entity.create Enemy\n")
            f.write("scene.save main\n")
            f.write("project.build\n")
            f.write("exit\n")
        
        print("Running full build...")
        result = subprocess.run(
            ["./game", "--headless", "--script", script_name],
            capture_output=True,
            text=True,
            timeout=300
        )
        
        print(f"\nReturn code: {result.returncode}")
        print(f"\n=== STDOUT (last 2000 chars) ===")
        print(result.stdout[-2000:])
        print(f"\n=== STDERR ===")
        print(result.stderr)
        
        os.remove(script_name)
        
    finally:
        os.chdir(original_dir)

if __name__ == "__main__":
    test_full_build()