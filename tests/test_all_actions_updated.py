#!/usr/bin/env python3
"""Test that all GitHub Actions use latest versions."""

import os
import sys
from pathlib import Path

def run_tests():
    """Run all tests for GitHub Actions latest versions."""
    failed_tests = []
    passed_tests = []
    
    workflows_dir = Path(".github/workflows")
    
    # Test 1: Check checkout action version
    try:
        outdated_found = []
        for workflow_file in workflows_dir.glob("*.yml"):
            with open(workflow_file, 'r') as f:
                content = f.read()
                
                if "actions/checkout@v3" in content:
                    outdated_found.append(workflow_file.name)
        
        assert not outdated_found, \
            f"Outdated checkout@v3 found in: {', '.join(outdated_found)}"
        
        passed_tests.append("test_checkout_version")
    except AssertionError as e:
        failed_tests.append(("test_checkout_version", str(e)))
    
    # Test 2: Check cache action version
    try:
        outdated_found = []
        for workflow_file in workflows_dir.glob("*.yml"):
            with open(workflow_file, 'r') as f:
                content = f.read()
                
                if "actions/cache@v3" in content:
                    outdated_found.append(workflow_file.name)
        
        assert not outdated_found, \
            f"Outdated cache@v3 found in: {', '.join(outdated_found)}"
        
        passed_tests.append("test_cache_version")
    except AssertionError as e:
        failed_tests.append(("test_cache_version", str(e)))
    
    # Test 3: Check setup-python action version
    try:
        outdated_found = []
        for workflow_file in workflows_dir.glob("*.yml"):
            with open(workflow_file, 'r') as f:
                content = f.read()
                
                if "actions/setup-python@v4" in content:
                    outdated_found.append(workflow_file.name)
        
        assert not outdated_found, \
            f"Outdated setup-python@v4 found in: {', '.join(outdated_found)}"
        
        passed_tests.append("test_setup_python_version")
    except AssertionError as e:
        failed_tests.append(("test_setup_python_version", str(e)))
    
    # Test 4: Verify latest versions are used
    try:
        latest_versions = {
            "checkout": False,
            "cache": False,
            "setup-python": False
        }
        
        for workflow_file in workflows_dir.glob("*.yml"):
            with open(workflow_file, 'r') as f:
                content = f.read()
                
                if "actions/checkout@v4" in content:
                    latest_versions["checkout"] = True
                if "actions/cache@v4" in content:
                    latest_versions["cache"] = True
                if "actions/setup-python@v5" in content:
                    latest_versions["setup-python"] = True
        
        missing = [k for k, v in latest_versions.items() if not v]
        assert not missing, \
            f"Latest versions not found for: {', '.join(missing)}"
        
        passed_tests.append("test_uses_latest_versions")
    except AssertionError as e:
        failed_tests.append(("test_uses_latest_versions", str(e)))
    
    # Print results
    print(f"\n{'='*60}")
    print("All Actions Update Tests")
    print(f"{'='*60}\n")
    
    if passed_tests:
        print(f"✅ Passed ({len(passed_tests)}):")
        for test in passed_tests:
            print(f"   - {test}")
    
    if failed_tests:
        print(f"\n❌ Failed ({len(failed_tests)}):")
        for test_name, error in failed_tests:
            print(f"   - {test_name}: {error}")
    
    print(f"\n{'='*60}")
    print(f"Total: {len(passed_tests)} passed, {len(failed_tests)} failed")
    print(f"{'='*60}\n")
    
    return len(failed_tests)

if __name__ == "__main__":
    sys.exit(run_tests())