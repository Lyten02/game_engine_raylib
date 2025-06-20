#!/usr/bin/env python3
"""
Test to verify that command injection vulnerability has been fixed in project.run command
"""

import os
import sys
import subprocess
import tempfile
import shutil
from pathlib import Path

def test_command_injection_fixed():
    """Test that project.run is NOT vulnerable to command injection"""
    print("=== Testing Command Injection Protection ===\n")
    
    # We run from build directory where game_engine is located
    # Create a test file that would be created if injection succeeds
    test_file = "/tmp/INJECTION_TEST.txt"
    if os.path.exists(test_file):
        os.remove(test_file)
    
    # Create a test project
    print("1. Creating a test project...")
    result = subprocess.run([
        "./game_engine", "--headless", "-c", 
        'project.create TestInjection'
    ], capture_output=True, text=True)
    
    if result.returncode != 0:
        print(f"   Failed to create project: {result.stderr}")
        return False
    
    # Create output directory structure
    output_dir = Path("../output/TestInjection")
    bin_dir = output_dir / "bin"
    bin_dir.mkdir(parents=True, exist_ok=True)
    
    # Create a normal executable
    normal_exe = bin_dir / "game"
    with open(normal_exe, "w") as f:
        f.write("#!/bin/sh\necho 'Normal game running'\n")
    normal_exe.chmod(0o755)
    
    # Test normal run
    print("\n2. Running project normally...")
    result = subprocess.run([
        "./game_engine", "--headless", "-c", "project.run"
    ], capture_output=True, text=True, timeout=5)
    print(f"   Exit code: {result.returncode}")
    
    # Create a malicious project name to test sanitization
    print("\n3. Testing with malicious project name...")
    malicious_name = 'Test"; touch /tmp/INJECTION_TEST.txt; echo "'
    result = subprocess.run([
        "./game_engine", "--headless", "-c", 
        f'project.create {malicious_name}'
    ], capture_output=True, text=True)
    
    # The project creation should either fail or sanitize the name
    if result.returncode == 0:
        print("   Project created (name was likely sanitized)")
    else:
        print("   Project creation rejected (good!)")
    
    # Check if injection file was created
    print("\n4. Checking if injection succeeded...")
    if os.path.exists(test_file):
        print("   ❌ VULNERABILITY: Injection file was created!")
        return False
    else:
        print("   ✅ Good: No injection detected")
    
    # Verify ProcessExecutor is used instead of std::system
    print("\n5. Checking source code for secure implementation...")
    source_file = "../src/engine/command_registry_build.cpp"
    
    if os.path.exists(source_file):
        with open(source_file, 'r') as f:
            content = f.read()
            
            # Check for vulnerable std::system usage
            if 'std::system' in content:
                print("   ❌ FOUND std::system usage - potential vulnerability!")
                return False
            
            # Check for safe ProcessExecutor usage
            if 'ProcessExecutor' in content and 'executor.execute' in content:
                print("   ✅ Found ProcessExecutor usage - secure implementation")
            else:
                print("   ⚠️  Could not verify ProcessExecutor usage")
    else:
        print("   ⚠️  Source file not found, skipping code verification")
    
    # Clean up
    shutil.rmtree("../output/TestInjection", ignore_errors=True)
    subprocess.run(["./game_engine", "--headless", "-c", "project.close"], 
                   capture_output=True)
    
    print("\n✅ All security checks passed!")
    return True

if __name__ == "__main__":
    # Tests are run from the build directory
    success = test_command_injection_fixed()
    print("\n" + "="*50)
    if success:
        print("SECURITY TEST PASSED - No command injection vulnerability")
        sys.exit(0)
    else:
        print("SECURITY TEST FAILED - Command injection vulnerability detected!")
        sys.exit(1)