#!/usr/bin/env python3
"""Check what files are created during build"""

import os
import subprocess
import time
import shutil

def run_command(cmd):
    """Run a shell command"""
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    return result.returncode == 0, result.stdout, result.stderr

# Clean up
if os.path.exists("projects/CheckBuild"):
    shutil.rmtree("projects/CheckBuild")
if os.path.exists("output/CheckBuild"):
    shutil.rmtree("output/CheckBuild")

# Create batch file
with open("check_build.txt", "w") as f:
    f.write("project.create CheckBuild\n")
    f.write("project.open CheckBuild\n")
    f.write("scene.create main\n")
    f.write("entity.create Player\n")
    f.write("scene.save main\n")
    f.write("project.build.sync\n")

print("Running build commands...")
success, stdout, stderr = run_command("../build/game_engine --headless --script check_build.txt")

print(f"Success: {success}")
print(f"stderr: {stderr}")

# Check what was created
print("\n=== Checking created files ===")

if os.path.exists("output/CheckBuild"):
    print("\nContents of output/CheckBuild:")
    for root, dirs, files in os.walk("output/CheckBuild"):
        level = root.replace("output/CheckBuild", "").count(os.sep)
        indent = " " * 2 * level
        print(f"{indent}{os.path.basename(root)}/")
        subindent = " " * 2 * (level + 1)
        for file in files[:10]:  # Limit to first 10 files
            print(f"{subindent}{file}")
        if len(files) > 10:
            print(f"{subindent}... and {len(files)-10} more files")
            
    # Check specific files
    print("\n=== Checking key files ===")
    
    # Check CMakeLists.txt
    cmake_file = "output/CheckBuild/CMakeLists.txt"
    if os.path.exists(cmake_file):
        print(f"✓ CMakeLists.txt exists ({os.path.getsize(cmake_file)} bytes)")
    else:
        print("✗ CMakeLists.txt NOT FOUND")
        
    # Check main.cpp
    main_file = "output/CheckBuild/main.cpp"
    if os.path.exists(main_file):
        print(f"✓ main.cpp exists ({os.path.getsize(main_file)} bytes)")
    else:
        print("✗ main.cpp NOT FOUND")
        
    # Check if cmake was run
    cmake_cache = "output/CheckBuild/build/CMakeCache.txt"
    if os.path.exists(cmake_cache):
        print(f"✓ CMake was run (CMakeCache.txt exists)")
    else:
        print("✗ CMake was NOT run (no CMakeCache.txt)")
        
    # Check for executable
    exe_path = "output/CheckBuild/bin/CheckBuild"
    if os.path.exists(exe_path):
        print(f"✓ Executable exists ({os.path.getsize(exe_path)} bytes)")
    else:
        print("✗ Executable NOT FOUND")
        
else:
    print("output/CheckBuild directory does not exist!")

# Clean up
os.remove("check_build.txt")
if os.path.exists("projects/CheckBuild"):
    shutil.rmtree("projects/CheckBuild")
if os.path.exists("output/CheckBuild"):  
    shutil.rmtree("output/CheckBuild")