#!/usr/bin/env python3
"""Test ResourceManager in headless mode"""

import subprocess
import json
import sys
import os

def test_headless_resource_loading():
    """Test that resource manager works in headless mode"""
    print("Testing headless resource manager...")
    
    exe_path = "./game_engine" if os.path.exists("./game_engine") else "./build/game_engine"
    
    # Test basic headless operation
    result = subprocess.run(
        [exe_path, "--json", "--headless", "--command", "help"],
        capture_output=True,
        text=True,
        timeout=10
    )
    
    assert result.returncode == 0, f"Headless mode failed: {result.stderr}"
    
    try:
        json_result = json.loads(result.stdout)
        assert json_result["success"] == True, "Help command failed in headless"
        print("✅ Basic headless operation works")
    except json.JSONDecodeError:
        assert False, f"Invalid JSON output: {result.stdout}"

def test_entity_creation_headless():
    """Test entity creation (which uses ResourceManager) in headless"""
    print("Testing entity creation in headless mode...")
    
    exe_path = "./game_engine" if os.path.exists("./game_engine") else "./build/game_engine"
    
    # Use a unique project name to avoid conflicts
    import time
    project_name = f"HeadlessTest_{int(time.time())}"
    
    commands = [
        f"project.create {project_name}",
        f"project.open {project_name}", 
        "entity.create TestEntity",
        "entity.list"
    ]
    
    batch_file = "test_headless_batch.txt"
    with open(batch_file, "w") as f:
        for cmd in commands:
            f.write(cmd + "\n")
    
    try:
        result = subprocess.run(
            [exe_path, "--json", "--headless", "--script", batch_file],
            capture_output=True,
            text=True,
            timeout=15
        )
        
        os.remove(batch_file)
        
        assert result.returncode == 0, f"Entity creation failed: {result.stderr}"
        
        try:
            json_result = json.loads(result.stdout)
        except json.JSONDecodeError:
            print(f"Invalid JSON output: {result.stdout}")
            raise
            
        assert json_result["success"] == True, f"Batch commands failed: {json_result}"
        print("✅ Entity creation works in headless mode")
        
    except Exception as e:
        if os.path.exists(batch_file):
            os.remove(batch_file)
        raise e

if __name__ == "__main__":
    try:
        # Find the game executable
        if os.path.exists("./game_engine"):
            # Already in build directory
            pass
        elif os.path.exists("./build/game_engine"):
            # In project root
            os.chdir("build")
        elif os.path.exists("../build/game_engine"):
            # In tests directory
            os.chdir("../build")
        else:
            raise Exception("Could not find game executable")
            
        test_headless_resource_loading()
        test_entity_creation_headless()
        
        print("\n🎉 All headless ResourceManager tests passed!")
        sys.exit(0)
        
    except Exception as e:
        print(f"\n❌ Test failed: {e}")
        sys.exit(1)