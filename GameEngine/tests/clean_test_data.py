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
        "batch_test"
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
    
    # Remove test results if exists
    if os.path.exists("test_results.json"):
        os.remove("test_results.json")
        print("  Removed: test_results.json")

if __name__ == "__main__":
    clean_test_data()