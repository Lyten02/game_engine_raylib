#!/usr/bin/env python3
"""Run only Python tests, skipping C++ compilation tests"""

import subprocess
import sys
import os

# List of Python test files to run
TESTS = [
    "test_cli_basic.py",
    "test_cli_json.py", 
    "test_cli_output.py",
    "test_entity_create_returns_id.py",
    "test_scene_memory_safety.py",
    "test_build_system.py",
    "test_projects.py",
    "test_sync_build.py"
]

def run_test(test_file):
    """Run a single test file"""
    print(f"\n{'='*60}")
    print(f"Running {test_file}...")
    print('='*60)
    
    try:
        result = subprocess.run(
            [sys.executable, test_file],
            cwd=os.path.dirname(os.path.abspath(__file__)),
            capture_output=True,
            text=True,
            timeout=30  # 30 second timeout per test
        )
        
        if result.returncode == 0:
            print(f"✅ {test_file} PASSED")
            return True
        else:
            print(f"❌ {test_file} FAILED")
            if result.stdout:
                print("STDOUT:", result.stdout[-500:])  # Last 500 chars
            if result.stderr:
                print("STDERR:", result.stderr[-500:])  # Last 500 chars
            return False
            
    except subprocess.TimeoutExpired:
        print(f"⏱️ {test_file} TIMED OUT")
        return False
    except Exception as e:
        print(f"💥 {test_file} ERROR: {e}")
        return False

def main():
    """Run all Python tests"""
    print("Running Python tests for GameEngine...")
    
    passed = 0
    failed = 0
    
    for test in TESTS:
        if run_test(test):
            passed += 1
        else:
            failed += 1
    
    print(f"\n{'='*60}")
    print(f"Test Summary:")
    print(f"  Passed: {passed}")
    print(f"  Failed: {failed}")
    print(f"  Total:  {passed + failed}")
    print('='*60)
    
    return 0 if failed == 0 else 1

if __name__ == "__main__":
    sys.exit(main())