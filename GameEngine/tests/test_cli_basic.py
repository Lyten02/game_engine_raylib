#!/usr/bin/env python3
"""
Basic CLI test script for Claude Coder
Tests basic functionality of the GameEngine CLI
"""

import subprocess
import json
import sys
import os

def run_engine_command(command, project=None):
    """Execute engine command and return JSON result"""
    # Build path to executable
    exe_path = "./game"
    if not os.path.exists(exe_path):
        exe_path = "./build/game"
    
    cmd = [exe_path, "--json", "--headless"]
    
    if project:
        cmd.extend(["--project", project])
    
    cmd.extend(["--command", command])
    
    result = subprocess.run(cmd, capture_output=True, text=True)
    
    try:
        return json.loads(result.stdout)
    except json.JSONDecodeError:
        return {
            "success": False,
            "error": f"Invalid JSON output: {result.stdout}",
            "stderr": result.stderr
        }

def test_basic_commands():
    """Test basic CLI commands"""
    print("Testing GameEngine CLI...")
    
    # Test 1: Help command
    print("\n1. Testing help command...")
    result = run_engine_command("help")
    assert result["success"], f"Help command failed: {result}"
    print("✅ Help command working")
    
    # Test 2: Project list (should be empty or have test projects)
    print("\n2. Testing project list...")
    result = run_engine_command("project.list")
    assert result["success"], f"Project list failed: {result}"
    print("✅ Project list working")
    
    # Test 3: Create project with unique name
    print("\n3. Testing project creation...")
    import time
    test_proj_name = f"test_automation_{int(time.time())}"
    result = run_engine_command(f"project.create {test_proj_name}")
    assert result["success"], f"Project creation failed: {result}"
    print("✅ Project creation working")
    
    # Test 4: Open project
    print("\n4. Testing project open...")
    result = run_engine_command(f"project.open {test_proj_name}")
    assert result["success"], f"Project open failed: {result}"
    print("✅ Project open working")
    
    # Test 5: Test resource manager in headless (entity creation uses ResourceManager)
    print("\n5. Testing resource manager in headless...")
    result = run_engine_command("entity.create TestEntity")
    assert result["success"], f"Entity creation failed in headless: {result}"
    print("✅ Resource manager working in headless")
    
    print("\n✅ All basic tests passed!")
    return True

def test_batch_commands():
    """Test batch command execution"""
    print("\nTesting batch commands...")
    
    exe_path = "./game"
    if not os.path.exists(exe_path):
        exe_path = "./build/game"
    
    import time
    batch_proj_name = f"batch_test_{int(time.time())}"
    cmd = [exe_path, "--json", "--headless", "--batch",
           f"project.create {batch_proj_name}",
           "project.list"]
    
    result = subprocess.run(cmd, capture_output=True, text=True)
    
    try:
        json_result = json.loads(result.stdout)
        assert json_result["success"], f"Batch execution failed: {json_result}"
        assert json_result["data"]["successful_commands"] == 2, "Not all commands succeeded"
        print("✅ Batch execution working")
        return True
    except Exception as e:
        print(f"❌ Batch execution failed: {e}")
        return False

def test_resource_manager_headless():
    """Test ResourceManager works correctly in headless mode"""
    print("\nTesting ResourceManager in headless mode...")
    
    # First create a test project and scene
    import time
    test_proj = f"test_rm_headless_{int(time.time())}"
    result = run_engine_command(f"project.create {test_proj}")
    assert result["success"], f"Project creation failed: {result}"
    
    result = run_engine_command(f"project.open {test_proj}")
    assert result["success"], f"Project open failed: {result}"
    
    result = run_engine_command("scene.create test_scene")
    assert result["success"], f"Scene creation failed: {result}"
    
    # Test 1: Create multiple entities
    print("  - Creating multiple entities...")
    entity_count = 10
    for i in range(entity_count):
        result = run_engine_command(f"entity.create TestEntity{i}", project=test_proj)
        assert result["success"], f"Entity creation {i} failed: {result}"
    
    print(f"  ✅ Created {entity_count} entities successfully")
    
    # Test 2: Verify engine still responsive
    result = run_engine_command("entity.list", project=test_proj)
    assert result["success"], "Entity list failed after entity creation"
    if "data" in result and result["data"] and "entities" in result["data"]:
        assert len(result["data"]["entities"]) >= entity_count, f"Should have at least {entity_count} entities"
    
    print("  ✅ Engine remains responsive in headless mode")
    
    # Test 3: Save and load scene (tests ResourceManager indirectly)
    result = run_engine_command("scene.save test_scene", project=test_proj)
    assert result["success"], "Scene save failed"
    print("  ✅ Scene saved successfully")
    
    result = run_engine_command("scene.load test_scene", project=test_proj)
    assert result["success"], "Scene load failed"
    print("  ✅ Scene loaded successfully")
    
    print("✅ ResourceManager works correctly in headless mode")
    return True

if __name__ == "__main__":
    try:
        # Change to build directory if we're in the project root
        if os.path.exists("build/game"):
            os.chdir("build")
        
        success = test_basic_commands()
        success = test_batch_commands() and success
        success = test_resource_manager_headless() and success
        
        if success:
            print("\n🎉 All CLI tests passed!")
            sys.exit(0)
        else:
            print("\n❌ Some tests failed")
            sys.exit(1)
            
    except Exception as e:
        print(f"\n❌ Test error: {e}")
        sys.exit(1)