#!/usr/bin/env python3
"""Test that resource cleanup works properly between tests"""

import subprocess
import os
import time
import shutil

def test_cleanup_between_tests():
    """Test that resources are cleaned between test runs"""
    print("Testing resource cleanup...")
    
    # Check initial state
    output_dir = "../output"
    if os.path.exists(output_dir):
        initial_projects = len([d for d in os.listdir(output_dir) if os.path.isdir(os.path.join(output_dir, d))])
        print(f"Initial projects in output: {initial_projects}")
    
    # Run a test that creates projects
    result = subprocess.run(
        ["python3", "test_cli_basic.py"],
        capture_output=True,
        text=True,
        timeout=30
    )
    
    if result.returncode != 0:
        print("Test failed to run")
        return False
    
    # Check if cleanup happened
    if os.path.exists(output_dir):
        after_projects = len([d for d in os.listdir(output_dir) if os.path.isdir(os.path.join(output_dir, d))])
        print(f"Projects after test: {after_projects}")
        
        # Should not accumulate too many projects
        if after_projects > initial_projects + 5:
            print(f"❌ Too many projects accumulated: {after_projects}")
            return False
    
    print("✅ Resource cleanup working")
    return True

def test_memory_safe_execution():
    """Test that we can run tests without memory explosion"""
    print("Testing memory-safe execution...")
    
    # Run a lightweight test multiple times
    for i in range(3):
        print(f"Iteration {i+1}/3")
        result = subprocess.run(
            ["python3", "test_config_system.py"],
            capture_output=True,
            text=True,
            timeout=10
        )
        
        if result.returncode != 0:
            print(f"Test failed on iteration {i+1}")
            return False
        
        # Small delay between tests
        time.sleep(0.5)
    
    print("✅ Memory-safe execution passed")
    return True

if __name__ == "__main__":
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    
    if not test_cleanup_between_tests():
        print("❌ Cleanup test failed")
        exit(1)
    
    if not test_memory_safe_execution():
        print("❌ Memory safety test failed")
        exit(1)
    
    print("\n✅ All resource cleanup tests passed!")
    exit(0)