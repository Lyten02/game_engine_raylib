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
    
    # Test 3: Create project
    print("\n3. Testing project creation...")
    result = run_engine_command("project.create test_automation")
    assert result["success"], f"Project creation failed: {result}"
    print("✅ Project creation working")
    
    # Test 4: Open project
    print("\n4. Testing project open...")
    result = run_engine_command("project.open test_automation")
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
    
    cmd = [exe_path, "--json", "--headless", "--batch",
           "project.create batch_test",
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

if __name__ == "__main__":
    try:
        # Change to build directory if we're in the project root
        if os.path.exists("build/game"):
            os.chdir("build")
        
        success = test_basic_commands()
        success = test_batch_commands() and success
        
        if success:
            print("\n🎉 All CLI tests passed!")
            sys.exit(0)
        else:
            print("\n❌ Some tests failed")
            sys.exit(1)
            
    except Exception as e:
        print(f"\n❌ Test error: {e}")
        sys.exit(1)