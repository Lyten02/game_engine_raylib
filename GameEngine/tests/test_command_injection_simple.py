#!/usr/bin/env python3
"""
Simple test for command injection vulnerability in project.run command
"""

import os
import sys
import subprocess
import tempfile
import shutil
from pathlib import Path

def test_command_injection():
    """Test if project.run is vulnerable to command injection"""
    print("=== Testing Command Injection Vulnerability ===\n")
    
    # Create a test file that will be created if injection succeeds
    test_file = "/tmp/INJECTION_TEST.txt"
    if os.path.exists(test_file):
        os.remove(test_file)
    
    # Simple valid project name first
    print("1. Creating a normal project...")
    result = subprocess.run([
        "./game", "--headless", "-c", 
        'project.create TestProject'
    ], cwd="build", capture_output=True, text=True)
    print(f"   Result: {result.returncode}")
    
    # Create output directory and dummy executable
    output_dir = Path("output/TestProject")
    bin_dir = output_dir / "bin"
    bin_dir.mkdir(parents=True, exist_ok=True)
    
    # Create a malicious executable name that includes shell commands
    # This simulates what would happen if project name wasn't sanitized
    malicious_path = bin_dir / 'game"; echo INJECTED > /tmp/INJECTION_TEST.txt; echo "game'
    normal_path = bin_dir / "game"
    
    # Create normal executable
    with open(normal_path, "w") as f:
        f.write("#!/bin/sh\necho 'Normal game running'\n")
    normal_path.chmod(0o755)
    
    # Test 1: Normal run (should work)
    print("\n2. Running project normally...")
    result = subprocess.run([
        "./game", "--headless", "-c", "project.run"
    ], cwd="build", capture_output=True, text=True)
    print(f"   Exit code: {result.returncode}")
    print(f"   Output: {result.stdout[:100]}...")
    
    # Test 2: Simulate what would happen with malicious path
    # We'll manually test the vulnerable command
    print("\n3. Testing vulnerable command construction...")
    
    # This is what the vulnerable code does:
    # std::string command = "\"" + execPath + "\" &";
    # std::system(command.c_str());
    
    # Simulate malicious executable path
    exec_path = str(bin_dir / 'game"; echo INJECTED > /tmp/INJECTION_TEST.txt; echo "x')
    
    # Show what command would be constructed
    vulnerable_command = f'"{exec_path}" &'
    print(f"   Vulnerable command would be: {vulnerable_command}")
    
    # Test if std::system would execute this (don't actually run it)
    print("\n4. Checking if injection file was created...")
    if os.path.exists(test_file):
        print("   ❌ VULNERABILITY CONFIRMED: Injection succeeded!")
        print(f"   File {test_file} was created")
        return False
    else:
        print("   ✓ No injection detected (yet)")
    
    # Clean up
    shutil.rmtree(output_dir, ignore_errors=True)
    subprocess.run(["./game", "--headless", "-c", "project.close"], cwd="build")
    
    # Now let's check the actual implementation
    print("\n5. Checking source code for std::system usage...")
    source_file = "src/engine/command_registry_build.cpp"
    with open(source_file, 'r') as f:
        content = f.read()
        if 'std::system' in content:
            # Find the line
            lines = content.split('\n')
            for i, line in enumerate(lines):
                if 'std::system' in line:
                    print(f"   ❌ FOUND std::system at line {i+1}: {line.strip()}")
                    print("   This is vulnerable to command injection!")
                    return False
    
    print("\n✓ All tests passed - no vulnerabilities found")
    return True

if __name__ == "__main__":
    os.chdir(Path(__file__).parent.parent)  # Go to GameEngine directory
    success = test_command_injection()
    print("\n" + "="*50)
    if success:
        print("SECURITY TEST PASSED")
        sys.exit(0)
    else:
        print("SECURITY TEST FAILED - Command injection vulnerability detected!")
        sys.exit(1)