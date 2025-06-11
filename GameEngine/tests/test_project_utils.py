#!/usr/bin/env python3
"""
Test utilities for preventing race conditions and ensuring cleanup
"""

import os
import shutil
import time
import random
import atexit
import subprocess
import json
from typing import List, Dict, Any

class TestProjectManager:
    """Manages test projects to prevent race conditions and ensure cleanup"""
    
    def __init__(self):
        self.created_projects: List[str] = []
        self.projects_dir = "projects"
        # Register cleanup on exit
        atexit.register(self.cleanup_all)
        
    def generate_unique_name(self, base_name: str) -> str:
        """Generate unique project name to avoid conflicts"""
        timestamp = int(time.time())
        random_suffix = random.randint(1000, 9999)
        unique_name = f"{base_name}_{timestamp}_{random_suffix}"
        self.created_projects.append(unique_name)
        return unique_name
    
    def cleanup_all(self):
        """Clean up all test projects created during this test run"""
        if not self.created_projects:
            return
            
        print(f"\n🧹 Cleaning up {len(self.created_projects)} test projects...")
        removed_count = 0
        
        for project_name in self.created_projects:
            if self.remove_project(project_name):
                removed_count += 1
                
        print(f"✅ Cleaned up {removed_count} test projects")
        self.created_projects.clear()
    
    def remove_project(self, project_name: str) -> bool:
        """Remove a specific project"""
        project_path = os.path.join(self.projects_dir, project_name)
        if os.path.exists(project_path):
            try:
                shutil.rmtree(project_path)
                return True
            except Exception as e:
                print(f"  ⚠️ Failed to remove {project_name}: {e}")
                return False
        return False
    
    def pre_test_cleanup(self):
        """Clean up any leftover test projects from previous runs"""
        if not os.path.exists(self.projects_dir):
            return
            
        print("🧹 Pre-test cleanup...")
        test_patterns = [
            "test_",
            "cli_test",
            "batch_test",
            "automation_test",
            "ResourceTest",
            "TimeoutTestProject",
            "CheckBuild",
            "BuildTest"
        ]
        
        removed = 0
        try:
            for item in os.listdir(self.projects_dir):
                if any(item.startswith(pattern) for pattern in test_patterns):
                    if self.remove_project(item):
                        removed += 1
                        
            if removed > 0:
                print(f"  Removed {removed} leftover test projects")
        except Exception as e:
            print(f"  ⚠️ Pre-cleanup failed: {e}")

# Global instance
project_manager = TestProjectManager()

def generate_unique_project_name(base_name: str) -> str:
    """Generate unique project name to avoid conflicts"""
    return project_manager.generate_unique_name(base_name)

def cleanup_test_projects():
    """Clean up all test projects"""
    project_manager.cleanup_all()

def pre_test_cleanup():
    """Clean up leftover test projects"""
    project_manager.pre_test_cleanup()

def create_or_open_project(game_exe, project_name):
    """Create a project if it doesn't exist, otherwise just open it"""
    # Check if project exists
    projects_dir = "projects" if os.path.exists("projects") else "../projects"
    project_exists = os.path.exists(os.path.join(projects_dir, project_name))
    
    if project_exists:
        # Project exists, just open it
        result = subprocess.run(
            [game_exe, "--json", "-c", f"project.open {project_name}"],
            capture_output=True,
            text=True
        )
    else:
        # Create new project
        result = subprocess.run(
            [game_exe, "--json", "-c", f"project.create {project_name}"],
            capture_output=True,
            text=True
        )
        if result.returncode == 0:
            # Open the newly created project
            result = subprocess.run(
                [game_exe, "--json", "-c", f"project.open {project_name}"],
                capture_output=True,
                text=True
            )
    
    return result

def get_build_command(project_name, force_full=False):
    """Get appropriate build command based on project state"""
    # Check if project has cached dependencies
    output_dirs = ["output", "../output"]
    
    has_cached_deps = False
    for output_dir in output_dirs:
        deps_path = os.path.join(output_dir, project_name, "build", "_deps")
        if os.path.exists(deps_path):
            has_cached_deps = True
            break
    
    if has_cached_deps and not force_full:
        return "project.build.fast"
    else:
        return "project.build"  # Use full build when no cached deps

def check_project_exists(project_name):
    """Check if a project exists in any of the common locations"""
    possible_dirs = ["projects", "../projects", "build/projects"]
    
    for proj_dir in possible_dirs:
        if os.path.exists(os.path.join(proj_dir, project_name)):
            return True
    return False

def check_output_exists(project_name):
    """Check if build output exists for a project"""
    possible_dirs = ["output", "../output", "build/output"]
    
    for out_dir in possible_dirs:
        if os.path.exists(os.path.join(out_dir, project_name)):
            return True
    return False
