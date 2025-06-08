#!/usr/bin/env python3
"""Test to check if build actually produces output despite warnings"""

import subprocess
import os
import sys
import time
import shutil

def test_build_output():
    """Test if build produces output files despite CMake warnings"""
    print("=== Build Output Test ===\n")
    
    # Move to build directory
    original_dir = os.getcwd()
    if os.path.basename(os.getcwd()) == "tests":
        os.chdir("../build")
    elif not os.path.exists("game"):
        if os.path.exists("build/game"):
            os.chdir("build")
    
    try:
        project_name = "BuildOutputTest"
        
        # Clean up
        if os.path.exists(f"projects/{project_name}"):
            shutil.rmtree(f"projects/{project_name}", ignore_errors=True)
        if os.path.exists(f"output/{project_name}"):
            shutil.rmtree(f"output/{project_name}", ignore_errors=True)
        
        # Create script
        script_name = "build_output_test.txt"
        with open(script_name, "w") as f:
            f.write(f"project.create {project_name}\n")
            f.write(f"project.open {project_name}\n")
            f.write("scene.create main\n")
            f.write("entity.create Player\n")
            f.write("scene.save main\n")
            f.write("project.build-fast\n")
            f.write("exit\n")
        
        print("Running build (ignoring return code)...")
        result = subprocess.run(
            ["./game", "--headless", "--script", script_name],
            capture_output=True,
            text=True,
            timeout=30
        )
        
        print(f"\nReturn code: {result.returncode}")
        
        # Check output files regardless of return code
        output_dir = f"output/{project_name}"
        print(f"\nChecking output directory: {output_dir}")
        
        if os.path.exists(output_dir):
            print("✅ Output directory exists")
            
            # List all files in output
            for root, dirs, files in os.walk(output_dir):
                level = root.replace(output_dir, '').count(os.sep)
                indent = ' ' * 2 * level
                print(f"{indent}{os.path.basename(root)}/")
                subindent = ' ' * 2 * (level + 1)
                for file in files:
                    size = os.path.getsize(os.path.join(root, file))
                    print(f"{subindent}{file} ({size:,} bytes)")
            
            # Check for executable in various locations
            possible_exe_paths = [
                f"{output_dir}/build/{project_name}",
                f"{output_dir}/bin/{project_name}",
                f"{output_dir}/{project_name}"
            ]
            
            exe_found = False
            for exe_path in possible_exe_paths:
                if os.path.exists(exe_path):
                    print(f"\n✅ Executable found: {exe_path}")
                    exe_found = True
                    break
            
            if not exe_found:
                print("\n❌ No executable found in expected locations")
        else:
            print("❌ Output directory does not exist")
        
        # Show last part of stderr to see warnings
        if result.stderr:
            print("\n=== Last 1000 chars of stderr ===")
            print(result.stderr[-1000:])
        
        os.remove(script_name)
        
    finally:
        os.chdir(original_dir)

if __name__ == "__main__":
    test_build_output()