#!/usr/bin/env python3
"""Test the build system functionality"""

import subprocess
import json
import sys
import os
import time
import shutil

def run_batch_commands(commands):
    """Execute multiple commands in one engine session"""
    # Change to build directory where templates are accessible
    original_dir = os.getcwd()
    
    # Move to build directory if we're in tests
    if os.path.basename(os.getcwd()) == "tests":
        os.chdir("../build")
    elif not os.path.exists("game"):
        # Try to find build directory
        if os.path.exists("build/game"):
            os.chdir("build")
    
    exe_path = "./game"
    
    # Write commands to a temporary batch file
    batch_file = "test_build_commands.txt"
    with open(batch_file, "w") as f:
        for cmd in commands:
            f.write(cmd + "\n")
    
    try:
        # Run engine with batch file
        result = subprocess.run(
            [exe_path, "--headless", "--script", batch_file],
            capture_output=True,
            text=True,
            timeout=150  # 2.5 minutes timeout for build
        )
        
        # Clean up batch file
        os.remove(batch_file)
        
        # Return to original directory
        os.chdir(original_dir)
        
        return result.returncode == 0, result.stdout, result.stderr
        
    except subprocess.TimeoutExpired:
        os.remove(batch_file)
        os.chdir(original_dir)
        return False, "", "Build timeout after 2.5 minutes"
    except Exception as e:
        if os.path.exists(batch_file):
            os.remove(batch_file)
        os.chdir(original_dir)
        return False, "", str(e)

def test_project_build():
    """Test full project build functionality"""
    print("Testing project build system...")
    print("This will compile a real project (may take 1-2 minutes)...")
    
    project_name = "BuildTest"
    
    # Determine paths based on current directory
    if os.path.basename(os.getcwd()) == "tests":
        project_dir = f"../build/projects/{project_name}"
        output_dir = f"../build/output/{project_name}"
    else:
        project_dir = f"projects/{project_name}"
        output_dir = f"output/{project_name}"
    
    # Clean up any existing project
    if os.path.exists(project_dir):
        shutil.rmtree(project_dir)
    if os.path.exists(output_dir):
        shutil.rmtree(output_dir)
    
    # Commands to execute in batch
    commands = [
        f"project.create {project_name}",
        f"project.open {project_name}",
        "scene.create main",
        "entity.create Player",
        "scene.save main",
        "project.build.sync"  # Use synchronous build
    ]
    
    print("\nExecuting batch commands:")
    for i, cmd in enumerate(commands, 1):
        print(f"  {i}. {cmd}")
    
    start_time = time.time()
    
    # Execute all commands in one session
    success, stdout, stderr = run_batch_commands(commands)
    
    elapsed = time.time() - start_time
    
    if not success:
        print(f"\n❌ Batch execution failed!")
        print(f"stdout: {stdout[:1000]}...")  # First 1000 chars
        print(f"stderr: {stderr}")
        assert False, "Failed to execute build commands"
    
    print(f"\n✅ Commands executed successfully in {elapsed:.1f}s")
    
    # Check if build was mentioned in output
    if "Build completed successfully" in stdout:
        print("✅ Build reported as successful")
    elif "Build failed" in stdout:
        print("❌ Build reported as failed")
        print(f"Build output:\n{stdout[-2000:]}")  # Last 2000 chars
        assert False, "Build failed"
    else:
        # Print last part of output to see what happened
        print("\nLast 500 chars of output:")
        print(stdout[-500:] if len(stdout) > 500 else stdout)
    
    # Verify executable was created
    if os.path.basename(os.getcwd()) == "tests":
        exe_path = f"../build/output/{project_name}/bin/{project_name}"
    else:
        exe_path = f"output/{project_name}/bin/{project_name}"
    
    if os.name == 'nt':
        exe_path += ".exe"
    
    # Give it a moment for files to be written
    time.sleep(1)
    
    if os.path.exists(exe_path):
        file_size = os.path.getsize(exe_path)
        print(f"✅ Executable found: {exe_path} ({file_size:,} bytes)")
        assert file_size > 100000, f"Executable too small ({file_size} bytes), likely not compiled correctly"
    else:
        # List what's in the output directory
        print(f"\n❌ Executable not found at: {exe_path}")
        print("\nChecking output directory structure:")
        
        # Check the correct output directory
        check_dir = "../build/output" if os.path.basename(os.getcwd()) == "tests" else "output"
        if os.path.exists(check_dir):
            for root, dirs, files in os.walk(check_dir):
                level = root.replace(check_dir, "").count(os.sep)
                indent = " " * 2 * level
                print(f"{indent}{os.path.basename(root)}/")
                subindent = " " * 2 * (level + 1)
                for file in files[:5]:  # Limit to 5 files per dir
                    print(f"{subindent}{file}")
                if len(files) > 5:
                    print(f"{subindent}... and {len(files)-5} more files")
        else:
            print(f"Output directory does not exist at: {check_dir}")
            
        assert False, f"Executable not found at: {exe_path}"

if __name__ == "__main__":
    print("=== Game Engine Build System Tests ===\n")
    
    try:
        test_project_build()
        print("\n🎉 All tests passed!")
        print("\nBuild system successfully:")
        print("  - Created project")
        print("  - Generated C++ code")
        print("  - Compiled with CMake")
        print("  - Produced executable")
        sys.exit(0)
    except AssertionError as e:
        print(f"\n❌ Test failed: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"\n❌ Unexpected error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)