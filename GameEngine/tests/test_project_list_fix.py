#!/usr/bin/env python3
"""Test that project.list command correctly finds projects"""

import os
import sys
import subprocess
import json
import shutil

def test_project_list_finds_existing_projects():
    """Test that project.list shows all existing projects"""
    print("Testing project.list command...")
    
    # Run from build directory
    original_dir = os.getcwd()
    build_dir = os.path.join(os.path.dirname(__file__), '..', 'build')
    os.chdir(build_dir)
    
    try:
        # Check what projects exist in ../projects
        projects_dir = "../projects"
        existing_projects = []
        if os.path.exists(projects_dir):
            for item in os.listdir(projects_dir):
                item_path = os.path.join(projects_dir, item)
                if os.path.isdir(item_path):
                    project_json = os.path.join(item_path, "project.json")
                    if os.path.exists(project_json):
                        existing_projects.append(item)
        
        print(f"Found {len(existing_projects)} existing projects: {existing_projects}")
        
        # Run project.list command
        result = subprocess.run(
            ["./game_engine", "--json", "--headless", "--command", "project.list"],
            capture_output=True,
            text=True
        )
        
        assert result.returncode == 0, f"Command failed: {result.stderr}"
        
        # Parse JSON response
        response = json.loads(result.stdout)
        output = response.get("output", "")
        
        # Check if it found any projects
        if "No projects found" in output:
            if existing_projects:
                print(f"✗ project.list shows 'No projects found' but {len(existing_projects)} projects exist!")
                print(f"  Existing projects: {existing_projects}")
                return False
            else:
                print("✓ No projects exist and project.list correctly shows none")
                return True
        
        # Extract project names from output
        listed_projects = []
        lines = output.strip().split('\n')
        for line in lines:
            if line.strip().startswith("- "):
                project_name = line.strip()[2:].strip()
                listed_projects.append(project_name)
        
        print(f"project.list returned {len(listed_projects)} projects: {listed_projects}")
        
        # Check if all existing projects are listed
        missing = set(existing_projects) - set(listed_projects)
        extra = set(listed_projects) - set(existing_projects)
        
        if missing:
            print(f"✗ Missing projects in list: {missing}")
        if extra:
            print(f"✗ Extra projects in list that don't exist: {extra}")
        
        if not missing and not extra:
            print("✓ project.list correctly shows all existing projects")
            return True
        else:
            return False
            
    finally:
        os.chdir(original_dir)

def test_project_open_can_open_listed_projects():
    """Test that projects shown in project.list can actually be opened"""
    print("\nTesting that listed projects can be opened...")
    
    # Run from build directory
    original_dir = os.getcwd()
    build_dir = os.path.join(os.path.dirname(__file__), '..', 'build')
    os.chdir(build_dir)
    
    try:
        # Get project list
        result = subprocess.run(
            ["./game_engine", "--json", "--headless", "--command", "project.list"],
            capture_output=True,
            text=True
        )
        
        response = json.loads(result.stdout)
        output = response.get("output", "")
        
        # Extract project names
        listed_projects = []
        if "No projects found" not in output:
            lines = output.strip().split('\n')
            for line in lines:
                if line.strip().startswith("- "):
                    project_name = line.strip()[2:].strip()
                    listed_projects.append(project_name)
        
        if not listed_projects:
            print("No projects listed to test opening")
            return True
        
        # Try to open each listed project
        failed_opens = []
        for project in listed_projects:
            result = subprocess.run(
                ["./game_engine", "--json", "--headless", "--command", f"project.open {project}"],
                capture_output=True,
                text=True
            )
            
            response = json.loads(result.stdout)
            if not response.get("success", False) or "Failed to open" in response.get("output", ""):
                failed_opens.append(project)
                print(f"✗ Failed to open project '{project}' that was listed")
        
        if failed_opens:
            print(f"✗ Could not open {len(failed_opens)} listed projects: {failed_opens}")
            return False
        else:
            print(f"✓ All {len(listed_projects)} listed projects can be opened")
            return True
            
    finally:
        os.chdir(original_dir)

def main():
    print("=== Project List Command Tests ===\n")
    
    tests = [
        test_project_list_finds_existing_projects,
        test_project_open_can_open_listed_projects
    ]
    
    passed = sum(test() for test in tests)
    total = len(tests)
    
    print(f"\n=== Results: {passed}/{total} tests passed ===")
    
    return 0 if passed == total else 1

if __name__ == "__main__":
    sys.exit(main())