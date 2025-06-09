#!/usr/bin/env python3
"""Test script to verify log reduction in ResourceManager"""

import subprocess
import sys
import os
import re

def count_log_patterns(output):
    """Count occurrences of specific log patterns"""
    patterns = {
        "headless_mode": r"\[ResourceManager\] Headless mode: using dummy texture",
        "default_texture": r"\[ResourceManager\] Using default texture",
        "not_found": r"Texture '.*' not found",
        "raylib_not_init": r"\[ResourceManager\] RayLib not initialized",
        "created_dummy": r"\[ResourceManager\] Created dummy texture"
    }
    
    counts = {}
    for name, pattern in patterns.items():
        counts[name] = len(re.findall(pattern, output))
    
    return counts

def test_single_run():
    """Test a single run to see log output"""
    print("Testing single entity creation...")
    
    result = subprocess.run([
        "./game", "--headless", "--command", "entity.create"
    ], capture_output=True, text=True, cwd="../build")
    
    counts = count_log_patterns(result.stderr)
    print(f"Log counts: {counts}")
    
    return counts

def test_multiple_entities():
    """Test creating multiple entities to see if logs are limited"""
    print("\nTesting multiple entity creation (20 entities)...")
    
    # Create a command file with multiple entity.create commands
    commands = "\n".join(["entity.create"] * 20)
    
    result = subprocess.run([
        "./game", "--headless", "--batch"
    ], input=commands, capture_output=True, text=True, cwd="../build")
    
    counts = count_log_patterns(result.stderr)
    print(f"Log counts: {counts}")
    
    # Check that logs are limited
    for pattern, count in counts.items():
        if count > 10:
            print(f"  ⚠️  Pattern '{pattern}' appeared {count} times (expected <= 10)")
    
    return counts

def test_threading_logs():
    """Test the threading test to see log output"""
    print("\nTesting threading test log output...")
    
    if os.path.exists("../build/test_resource_manager_threading"):
        result = subprocess.run([
            "./test_resource_manager_threading"
        ], capture_output=True, text=True, cwd="../build")
        
        # Count all ResourceManager logs
        all_logs = len(re.findall(r"\[ResourceManager\]", result.stdout + result.stderr))
        print(f"Total ResourceManager logs: {all_logs}")
        
        if all_logs > 50:
            print(f"  ⚠️  Too many logs in threading test: {all_logs}")
        else:
            print(f"  ✅ Log count is reasonable")
    else:
        print("  ⚠️  Threading test not found, skipping")

def test_headless_test_logs():
    """Test the headless test to see log output"""
    print("\nTesting headless test log output...")
    
    if os.path.exists("../build/test_resource_manager_headless"):
        result = subprocess.run([
            "./test_resource_manager_headless"
        ], capture_output=True, text=True, cwd="../build")
        
        # The test should mostly use silent mode
        all_logs = len(re.findall(r"\[ResourceManager\]", result.stdout + result.stderr))
        print(f"Total ResourceManager logs: {all_logs}")
        
        # Should be minimal since test uses setSilentMode(true)
        if all_logs > 20:
            print(f"  ⚠️  Too many logs in headless test: {all_logs}")
        else:
            print(f"  ✅ Log count is minimal")
    else:
        print("  ⚠️  Headless test not found, skipping")

if __name__ == "__main__":
    print("=" * 60)
    print("ResourceManager Log Reduction Test")
    print("=" * 60)
    
    # Check if we're in the right directory
    if not os.path.exists("../build/game"):
        print("ERROR: Build directory not found. Please build the project first.")
        sys.exit(1)
    
    # Run tests
    test_single_run()
    test_multiple_entities()
    test_threading_logs()
    test_headless_test_logs()
    
    print("\n" + "=" * 60)
    print("Log reduction test completed!")
    print("=" * 60)