#!/usr/bin/env python3
"""Test that GitHub Actions use non-deprecated versions."""

import os
import sys
from pathlib import Path
import re

def run_tests():
    """Run all tests for GitHub Actions versions."""
    failed_tests = []
    passed_tests = []
    
    workflows_dir = Path(".github/workflows")
    
    # Test 1: Check upload-artifact version in all workflows
    try:
        deprecated_found = []
        for workflow_file in workflows_dir.glob("*.yml"):
            with open(workflow_file, 'r') as f:
                content = f.read()
                
                # Check for deprecated upload-artifact@v3
                if "actions/upload-artifact@v3" in content:
                    deprecated_found.append(workflow_file.name)
        
        assert not deprecated_found, \
            f"Deprecated upload-artifact@v3 found in: {', '.join(deprecated_found)}"
        
        passed_tests.append("test_upload_artifact_version")
    except AssertionError as e:
        failed_tests.append(("test_upload_artifact_version", str(e)))
    
    # Test 2: Check download-artifact version
    try:
        deprecated_found = []
        for workflow_file in workflows_dir.glob("*.yml"):
            with open(workflow_file, 'r') as f:
                content = f.read()
                
                # Check for deprecated download-artifact@v3
                if "actions/download-artifact@v3" in content:
                    deprecated_found.append(workflow_file.name)
        
        assert not deprecated_found, \
            f"Deprecated download-artifact@v3 found in: {', '.join(deprecated_found)}"
        
        passed_tests.append("test_download_artifact_version")
    except AssertionError as e:
        failed_tests.append(("test_download_artifact_version", str(e)))
    
    # Test 3: Check that v4 versions are used
    try:
        v4_found = False
        for workflow_file in workflows_dir.glob("*.yml"):
            with open(workflow_file, 'r') as f:
                content = f.read()
                
                if "actions/upload-artifact@v4" in content:
                    v4_found = True
                    break
        
        assert v4_found, "Should use upload-artifact@v4"
        passed_tests.append("test_uses_v4_artifact_actions")
    except AssertionError as e:
        failed_tests.append(("test_uses_v4_artifact_actions", str(e)))
    
    # Test 4: Check other actions are up to date
    try:
        outdated = []
        for workflow_file in workflows_dir.glob("*.yml"):
            with open(workflow_file, 'r') as f:
                content = f.read()
                
                # Check for other potentially outdated actions
                if "actions/checkout@v3" in content:
                    outdated.append(f"{workflow_file.name}: checkout@v3 (latest is v4)")
                if "actions/cache@v3" in content:
                    outdated.append(f"{workflow_file.name}: cache@v3 (latest is v4)")
                if "actions/setup-python@v4" in content:
                    outdated.append(f"{workflow_file.name}: setup-python@v4 (latest is v5)")
        
        # This is informational, not a failure
        if outdated:
            print(f"Info: Other actions could be updated:\n  " + "\n  ".join(outdated))
        
        passed_tests.append("test_other_actions_checked")
    except Exception as e:
        failed_tests.append(("test_other_actions_checked", str(e)))
    
    # Print results
    print(f"\n{'='*60}")
    print("GitHub Actions Version Tests")
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