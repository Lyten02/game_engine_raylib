#!/usr/bin/env python3
"""Test Scene memory management safety"""

import subprocess
import json
import sys
import os

# Find the executable
def find_executable():
    """Find the game executable"""
    # Get the directory where this script is located
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    # Check common paths relative to script location
    paths = [
        './game',  # If we're in build directory
        os.path.join(script_dir, '../build/game'),  # From tests directory
        os.path.join(script_dir, './build/game'),  # Alternative
        '../build/game',  # Relative to current dir
        './build/game'   # Another alternative
    ]
    
    for path in paths:
        if os.path.exists(path):
            return os.path.abspath(path)
    
    # If still not found, print debug info
    print(f"Current directory: {os.getcwd()}")
    print(f"Script directory: {script_dir}")
    print("Searched paths:")
    for path in paths:
        print(f"  - {path} (exists: {os.path.exists(path)})")
    
    raise FileNotFoundError("Cannot find game executable")

def test_scene_switching_safety():
    """Test that scene switching doesn't cause crashes"""
    game_path = find_executable()
    
    # Use timestamp to ensure unique project name
    import time
    project_name = f"SceneTest_{int(time.time())}"
    
    # Test rapid scene creation/switching
    commands = [
        f"project.create {project_name}",
        f"project.open {project_name}", 
        "scene.create scene1",
        "entity.create TestEntity1",
        "scene.create scene2", 
        "entity.create TestEntity2",
        "scene.create scene3",
        "entity.list",  # Should work without crash
        "project.close"
    ]
    
    for cmd in commands:
        result = subprocess.run(
            [game_path, "--json", "--headless", "--command", cmd],
            capture_output=True, text=True, timeout=10
        )
        
        if result.returncode != 0:
            print(f"Command failed: {cmd}")
            print(f"Return code: {result.returncode}")
            print(f"Stdout: {result.stdout}")
            print(f"Stderr: {result.stderr}")
            return False
    
    print("✅ Scene switching safety test passed")
    return True

def test_entity_commands_with_scene():
    """Test entity commands work correctly with new scene system"""
    game_path = find_executable()
    
    # Use timestamp to ensure unique project name
    import time
    project_name = f"EntityTest_{int(time.time())}"
    
    result = subprocess.run(
        [game_path, "--json", "--headless", "--batch",
         f"project.create {project_name}",
         f"project.open {project_name}",
         "scene.create main",
         "entity.create Player",
         "entity.list",
         "entity.create Enemy",
         "entity.list"],
        capture_output=True, text=True, timeout=15
    )
    
    if result.returncode != 0:
        print(f"Batch entity test failed: {result.stderr}")
        return False
    
    try:
        response = json.loads(result.stdout)
        if not response.get("success"):
            print(f"Entity test failed: {response}")
            return False
    except json.JSONDecodeError:
        print(f"Invalid JSON response: {result.stdout}")
        return False
    
    print("✅ Entity commands with scene test passed")
    return True

def test_no_memory_leaks():
    """Test for obvious memory issues"""
    game_path = find_executable()
    
    # Use timestamp to ensure unique project names
    import time
    timestamp = int(time.time())
    
    # Run multiple scene operations to stress test
    for i in range(10):
        result = subprocess.run(
            [game_path, "--json", "--headless", "--batch",
             f"project.create MemTest_{timestamp}_{i}",
             f"project.open MemTest_{timestamp}_{i}",
             "scene.create main",
             "entity.create Player",
             "entity.create Enemy", 
             "project.close"],
            capture_output=True, text=True, timeout=20
        )
        
        if result.returncode != 0:
            print(f"Memory test iteration {i} failed")
            return False
    
    print("✅ Memory stress test passed")
    return True

if __name__ == "__main__":
    print("Testing Scene memory safety...")
    
    tests = [
        test_scene_switching_safety,
        test_entity_commands_with_scene,
        test_no_memory_leaks
    ]
    
    for test in tests:
        if not test():
            print("❌ Scene memory safety tests failed!")
            sys.exit(1)
    
    print("\n🎉 All Scene memory safety tests passed!")
    sys.exit(0)