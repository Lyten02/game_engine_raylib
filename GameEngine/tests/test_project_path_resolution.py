#!/usr/bin/env python3
"""Test to understand how project paths are resolved"""

import os
import sys
import subprocess
import json

def test_engine_paths_info():
    """Display engine path information to understand resolution"""
    print("Testing engine path resolution...")
    
    # Run from build directory
    original_dir = os.getcwd()
    build_dir = os.path.join(os.path.dirname(__file__), '..', 'build')
    os.chdir(build_dir)
    
    try:
        # Create a simple command to display paths
        result = subprocess.run(
            ["./game_engine", "--json", "--headless", "--command", "engine.paths"],
            capture_output=True,
            text=True
        )
        
        # Even if command doesn't exist, let's check working directory
        print(f"Working directory: {os.getcwd()}")
        print(f"Absolute path: {os.path.abspath('.')}")
        print(f"Projects should be in: {os.path.abspath('../projects')}")
        print(f"Projects directory exists: {os.path.exists('../projects')}")
        
        if os.path.exists('../projects'):
            print("\nProjects found:")
            for item in os.listdir('../projects'):
                item_path = os.path.join('../projects', item)
                if os.path.isdir(item_path) and os.path.exists(os.path.join(item_path, 'project.json')):
                    print(f"  - {item}")
        
        return True
        
    finally:
        os.chdir(original_dir)

def main():
    print("=== Project Path Resolution Test ===\n")
    
    test_engine_paths_info()
    
    print("\nThis shows that when running from build directory:")
    print("- Working directory is: GameEngine/build")
    print("- Projects are in: GameEngine/projects (../projects from build)")
    print("- But getProjectList() looks for 'projects' (would be GameEngine/build/projects)")

if __name__ == "__main__":
    main()