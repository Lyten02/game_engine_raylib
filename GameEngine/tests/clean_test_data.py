#!/usr/bin/env python3
"""
Clean up test data before running tests
"""

import os
import shutil
import sys

def clean_test_data():
    """Remove test projects and data"""
    print("🧹 Cleaning test data...")
    
    # Find projects directory
    if os.path.exists("projects"):
        projects_dir = "projects"
    elif os.path.exists("build/projects"):
        projects_dir = "build/projects"
    else:
        print("Projects directory not found")
        return
    
    # List of test project patterns
    test_patterns = [
        "test_",
        "test",
        "cli_test",
        "automation_test",
        "batch_test",
        "BuildTest",
        "ConfigTest",
        "TestGame"
    ]
    
    # Remove test projects
    removed = 0
    for project in os.listdir(projects_dir):
        if any(project.startswith(pattern) for pattern in test_patterns):
            project_path = os.path.join(projects_dir, project)
            if os.path.isdir(project_path):
                shutil.rmtree(project_path)
                print(f"  Removed: {project}")
                removed += 1
    
    print(f"✅ Cleaned {removed} test projects")
    
    # Also clean output directory
    if os.path.exists("output"):
        output_dir = "output"
    elif os.path.exists("build/output"):
        output_dir = "build/output"
    else:
        output_dir = None
        
    if output_dir:
        for project in os.listdir(output_dir):
            if any(project.startswith(pattern) or project == pattern for pattern in test_patterns):
                project_path = os.path.join(output_dir, project)
                if os.path.isdir(project_path):
                    shutil.rmtree(project_path)
                    print(f"  Removed output: {project}")
    
    # Clean test directories if they exist
    test_dir = "tests" if os.path.exists("tests") else "../tests" if os.path.exists("../tests") else None
    if test_dir:
        # Remove test projects directory
        test_projects = os.path.join(test_dir, "projects")
        if os.path.exists(test_projects):
            shutil.rmtree(test_projects)
            print("  Removed: tests/projects")
        
        # Remove test output directory
        test_output = os.path.join(test_dir, "output")
        if os.path.exists(test_output):
            shutil.rmtree(test_output)
            print("  Removed: tests/output")
    
    # Remove test results if exists
    if os.path.exists("test_results.json"):
        os.remove("test_results.json")
        print("  Removed: test_results.json")

if __name__ == "__main__":
    clean_test_data()