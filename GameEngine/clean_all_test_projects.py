#!/usr/bin/env python3
"""
Clean all test projects from projects/ and output/ directories
"""

import os
import shutil
import re
from pathlib import Path

def is_test_project(name):
    """Check if a project name looks like a test project"""
    test_patterns = [
        r'^test',           # starts with test
        r'Test',            # contains Test
        r'_test_',          # contains _test_
        r'_\d{10}',         # contains timestamp (10 digits)
        r'^MemTest_',       # memory test projects
        r'^batch_test',     # batch test projects
        r'^EntityTest_',    # entity test projects
        r'^SceneTest_',     # scene test projects
        r'^HeadlessTest_',  # headless test projects
        r'^BuildTest',      # build test projects
        r'^FastBuildTest',  # fast build test projects
        r'^QuietProject',   # quiet test projects
        r'^BatchProject',   # batch projects
        r'^LogLevel',       # log level test projects
        r'^ResourceTest',   # resource test projects
        r'^TimeoutTest',    # timeout test projects
        r'^SecurityBuildTest',  # security test projects
    ]
    
    for pattern in test_patterns:
        if re.search(pattern, name, re.IGNORECASE):
            return True
    return False

def clean_test_projects():
    """Remove all test projects from projects and output directories"""
    
    removed_count = 0
    skipped_count = 0
    
    # Clean projects directory
    projects_dir = Path("projects")
    if projects_dir.exists():
        print("\n🧹 Cleaning projects directory...")
        for item in projects_dir.iterdir():
            if item.is_dir() and is_test_project(item.name):
                try:
                    shutil.rmtree(item)
                    print(f"  ✅ Removed: projects/{item.name}")
                    removed_count += 1
                except Exception as e:
                    print(f"  ❌ Failed to remove projects/{item.name}: {e}")
            else:
                if item.is_dir():
                    print(f"  ⏭️  Skipped: projects/{item.name}")
                    skipped_count += 1
    
    # Clean output directory
    output_dir = Path("output")
    if output_dir.exists():
        print("\n🧹 Cleaning output directory...")
        for item in output_dir.iterdir():
            if item.is_dir() and is_test_project(item.name):
                try:
                    shutil.rmtree(item)
                    print(f"  ✅ Removed: output/{item.name}")
                    removed_count += 1
                except Exception as e:
                    print(f"  ❌ Failed to remove output/{item.name}: {e}")
            else:
                if item.is_dir():
                    print(f"  ⏭️  Skipped: output/{item.name}")
                    skipped_count += 1
    
    # Also check parent directories if we're in build/
    if Path("../projects").exists():
        print("\n🧹 Cleaning ../projects directory...")
        for item in Path("../projects").iterdir():
            if item.is_dir() and is_test_project(item.name):
                try:
                    shutil.rmtree(item)
                    print(f"  ✅ Removed: ../projects/{item.name}")
                    removed_count += 1
                except Exception as e:
                    print(f"  ❌ Failed to remove ../projects/{item.name}: {e}")
    
    if Path("../output").exists():
        print("\n🧹 Cleaning ../output directory...")
        for item in Path("../output").iterdir():
            if item.is_dir() and is_test_project(item.name):
                try:
                    shutil.rmtree(item)
                    print(f"  ✅ Removed: ../output/{item.name}")
                    removed_count += 1
                except Exception as e:
                    print(f"  ❌ Failed to remove ../output/{item.name}: {e}")
    
    print(f"\n📊 Summary:")
    print(f"  - Removed: {removed_count} test projects")
    print(f"  - Skipped: {skipped_count} real projects")
    print("\n✨ Cleanup complete!")

if __name__ == "__main__":
    print("🧹 GameEngine Test Project Cleaner")
    print("=" * 50)
    print("This will remove all test projects while preserving real projects.")
    
    # Ask for confirmation
    response = input("\nProceed with cleanup? (y/N): ")
    if response.lower() == 'y':
        clean_test_projects()
    else:
        print("Cleanup cancelled.")