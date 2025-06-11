#!/usr/bin/env python3
"""
Utility functions for smart test cleanup that preserves dependency caches
"""

import os
import shutil
from pathlib import Path

def clean_test_project(project_name, preserve_cache=True):
    """
    Clean up a test project while optionally preserving dependency cache
    
    Args:
        project_name: Name of the project to clean
        preserve_cache: If True, preserves build/_deps directory
    """
    # Clean from both possible locations (running from build or tests dir)
    project_paths = [
        f"../projects/{project_name}",
        f"projects/{project_name}"
    ]
    
    output_paths = [
        f"../output/{project_name}",
        f"output/{project_name}"
    ]
    
    # Clean project directories
    for project_path in project_paths:
        if os.path.exists(project_path):
            try:
                shutil.rmtree(project_path)
                print(f"  ✅ Cleaned project: {project_path}")
            except Exception as e:
                print(f"  ⚠️  Failed to clean project {project_path}: {e}")
    
    # Clean output directories
    for output_path in output_paths:
        if os.path.exists(output_path):
            if preserve_cache:
                clean_output_preserve_cache(output_path)
            else:
                try:
                    shutil.rmtree(output_path)
                    print(f"  ✅ Cleaned output: {output_path}")
                except Exception as e:
                    print(f"  ⚠️  Failed to clean output {output_path}: {e}")

def clean_output_preserve_cache(output_path):
    """
    Clean output directory but preserve build/_deps cache
    """
    try:
        for item in os.listdir(output_path):
            item_path = os.path.join(output_path, item)
            
            if item == "build":
                # Clean build directory but keep _deps
                clean_build_preserve_deps(item_path)
            else:
                # Remove other files/directories
                if os.path.isfile(item_path):
                    os.remove(item_path)
                elif os.path.isdir(item_path):
                    shutil.rmtree(item_path)
        
        print(f"  ✅ Cleaned output (cache preserved): {output_path}")
    except Exception as e:
        print(f"  ⚠️  Failed to clean output {output_path}: {e}")

def clean_build_preserve_deps(build_path):
    """
    Clean build directory but preserve _deps subdirectory
    """
    if not os.path.exists(build_path):
        return
    
    for build_item in os.listdir(build_path):
        if build_item == "_deps":
            continue  # Preserve dependency cache
        
        build_item_path = os.path.join(build_path, build_item)
        try:
            if os.path.isfile(build_item_path):
                os.remove(build_item_path)
            elif os.path.isdir(build_item_path):
                shutil.rmtree(build_item_path)
        except Exception as e:
            print(f"  ⚠️  Failed to remove {build_item_path}: {e}")

def get_cache_size(project_name):
    """
    Get the size of the dependency cache for a project
    """
    cache_paths = [
        f"../output/{project_name}/build/_deps",
        f"output/{project_name}/build/_deps"
    ]
    
    for cache_path in cache_paths:
        if os.path.exists(cache_path):
            total_size = 0
            for dirpath, dirnames, filenames in os.walk(cache_path):
                for filename in filenames:
                    filepath = os.path.join(dirpath, filename)
                    total_size += os.path.getsize(filepath)
            return total_size
    
    return 0

def format_size(size_bytes):
    """
    Format size in bytes to human readable format
    """
    for unit in ['B', 'KB', 'MB', 'GB']:
        if size_bytes < 1024.0:
            return f"{size_bytes:.1f} {unit}"
        size_bytes /= 1024.0
    return f"{size_bytes:.1f} TB"

class TestProjectCleaner:
    """
    Context manager for automatic test project cleanup
    """
    def __init__(self, project_name, preserve_cache=True):
        self.project_name = project_name
        self.preserve_cache = preserve_cache
    
    def __enter__(self):
        # Clean up any previous test run
        clean_test_project(self.project_name, self.preserve_cache)
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        # Clean up after test
        print(f"\nCleaning up test project '{self.project_name}'...")
        clean_test_project(self.project_name, self.preserve_cache)
        
        # Report cache size if preserved
        if self.preserve_cache:
            cache_size = get_cache_size(self.project_name)
            if cache_size > 0:
                print(f"  ℹ️  Preserved cache size: {format_size(cache_size)}")