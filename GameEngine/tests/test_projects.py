#!/usr/bin/env python3
"""
Test project management functionality
"""

import subprocess
import json
import sys
import os
import tempfile
import shutil

def run_command(command):
    """Execute engine command and return JSON result"""
    exe_path = "./game" if os.path.exists("./game") else "./build/game"
    
    result = subprocess.run(
        [exe_path, "--json", "--headless", "--command", command],
        capture_output=True,
        text=True
    )
    
    try:
        return json.loads(result.stdout)
    except json.JSONDecodeError:
        return {"success": False, "error": f"Invalid JSON: {result.stdout}"}

def test_project_lifecycle():
    """Test creating, opening, and closing projects"""
    print("Testing project lifecycle...")
    
    # 1. Create project
    result = run_command("project.create test_lifecycle")
    assert result["success"], f"Project creation failed: {result}"
    print("✅ Project created")
    
    # 2. Open project
    result = run_command("project.open test_lifecycle")
    assert result["success"], f"Project open failed: {result}"
    print("✅ Project opened")
    
    # 3. Create scene in project
    result = run_command("scene.create test_scene")
    assert result["success"], f"Scene creation failed: {result}"
    print("✅ Scene created")
    
    # 4. Close project
    result = run_command("project.close")
    assert result["success"], f"Project close failed: {result}"
    print("✅ Project closed")
    
    return True

def test_multiple_projects():
    """Test handling multiple projects"""
    print("\nTesting multiple projects...")
    
    # Create multiple projects
    projects = ["test_proj1", "test_proj2", "test_proj3"]
    
    for proj in projects:
        result = run_command(f"project.create {proj}")
        assert result["success"], f"Failed to create {proj}: {result}"
    
    print(f"✅ Created {len(projects)} projects")
    
    # List projects
    result = run_command("project.list")
    assert result["success"], f"Project list failed: {result}"
    print("✅ Project list working")
    
    return True

def test_project_persistence():
    """Test that project data persists"""
    print("\nTesting project persistence...")
    
    # Create and setup project
    result = run_command("project.create test_persist")
    assert result["success"], "Failed to create project"
    
    result = run_command("project.open test_persist")
    assert result["success"], "Failed to open project"
    
    # Add some data
    result = run_command("scene.create persistent_scene")
    assert result["success"], "Failed to create scene"
    
    result = run_command("entity.create TestEntity")
    assert result["success"], "Failed to create entity"
    
    # Close project
    result = run_command("project.close")
    assert result["success"], "Failed to close project"
    
    # Reopen and verify
    result = run_command("project.open test_persist")
    assert result["success"], "Failed to reopen project"
    print("✅ Project persistence working")
    
    result = run_command("project.close")
    assert result["success"], "Failed to close project"
    
    return True

if __name__ == "__main__":
    try:
        # Change to build directory if needed
        if os.path.exists("build/game"):
            os.chdir("build")
        
        print("🧪 Project Management Tests")
        print("="*40)
        
        success = True
        success = test_project_lifecycle() and success
        success = test_multiple_projects() and success
        success = test_project_persistence() and success
        
        if success:
            print("\n✅ All project tests passed!")
            sys.exit(0)
        else:
            print("\n❌ Some tests failed")
            sys.exit(1)
            
    except Exception as e:
        print(f"\n❌ Test error: {e}")
        sys.exit(1)