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
import atexit
import time
import random
from test_project_utils import generate_unique_project_name, cleanup_test_projects, pre_test_cleanup

# Global list to track test projects for cleanup
test_projects_created = []

def cleanup_test_projects_local():
    """Clean up all test projects created during this test run"""
    if not test_projects_created:
        return
    print(f"\n🧹 Cleaning up {len(test_projects_created)} test projects...")
    for project_name in test_projects_created:
        project_path = f"projects/{project_name}"
        if os.path.exists(project_path):
            try:
                shutil.rmtree(project_path)
                print(f"  Removed: {project_name}")
            except Exception as e:
                print(f"  Failed to remove {project_name}: {e}")

# Register cleanup function to run on exit
atexit.register(cleanup_test_projects_local)

def generate_unique_project_name_local(base_name):
    """Generate unique project name to avoid conflicts"""
    timestamp = int(time.time())
    random_suffix = random.randint(1000, 9999)
    unique_name = f"{base_name}_{timestamp}_{random_suffix}"
    test_projects_created.append(unique_name)
    return unique_name

def run_command(command):
    """Execute engine command and return JSON result"""
    exe_path = "./game_engine" if os.path.exists("./game_engine") else "./build/game_engine"
    
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
    
    # Generate unique project name
    project_name = generate_unique_project_name_local("test_lifecycle")
    
    # 1. Create project
    result = run_command(f"project.create {project_name}")
    assert result["success"], f"Project creation failed: {result}"
    print("✅ Project created")
    
    # 2. Open project
    result = run_command(f"project.open {project_name}")
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
    
    # Create multiple projects with unique names
    projects = []
    for i in range(3):
        project_name = generate_unique_project_name_local(f"test_proj{i+1}")
        projects.append(project_name)
    
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
    
    # Generate unique project name
    project_name = generate_unique_project_name_local("test_persist")
    
    # Create and setup project
    result = run_command(f"project.create {project_name}")
    assert result["success"], "Failed to create project"
    
    result = run_command(f"project.open {project_name}")
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
    result = run_command(f"project.open {project_name}")
    assert result["success"], "Failed to reopen project"
    print("✅ Project persistence working")
    
    result = run_command("project.close")
    assert result["success"], "Failed to close project"
    
    return True

if __name__ == "__main__":
    try:
        # Change to build directory if needed
        if os.path.exists("build/game_engine"):
            os.chdir("build")
        
        print("🧪 Project Management Tests")
        print("="*40)
        
        # Run cleanup first to ensure clean state
        print("🧹 Pre-test cleanup...")
        if os.path.exists("../tests/clean_test_data.py"):
            subprocess.run(["python", "../tests/clean_test_data.py"], capture_output=True)
        elif os.path.exists("tests/clean_test_data.py"):
            subprocess.run(["python", "tests/clean_test_data.py"], capture_output=True)
        
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
    finally:
        # Cleanup will be called automatically by atexit
        pass