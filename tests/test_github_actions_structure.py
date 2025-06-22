#!/usr/bin/env python3
"""Test GitHub Actions structure and configuration."""

import os
import sys
from pathlib import Path

def run_tests():
    """Run all tests and report results."""
    failed_tests = []
    passed_tests = []
    
    # Test 1: .github directory exists
    try:
        github_dir = Path(".github")
        assert github_dir.exists(), ".github directory should exist"
        assert github_dir.is_dir(), ".github should be a directory"
        passed_tests.append("test_github_directory_exists")
    except AssertionError as e:
        failed_tests.append(("test_github_directory_exists", str(e)))
    
    # Test 2: .github/workflows directory exists
    try:
        workflows_dir = Path(".github/workflows")
        assert workflows_dir.exists(), ".github/workflows directory should exist"
        assert workflows_dir.is_dir(), ".github/workflows should be a directory"
        passed_tests.append("test_workflows_directory_exists")
    except AssertionError as e:
        failed_tests.append(("test_workflows_directory_exists", str(e)))
    
    # Test 3: CI workflow file exists
    try:
        ci_workflow = Path(".github/workflows/ci.yml")
        assert ci_workflow.exists(), "CI workflow file should exist"
        assert ci_workflow.is_file(), "ci.yml should be a file"
        passed_tests.append("test_base_workflow_file_exists")
    except AssertionError as e:
        failed_tests.append(("test_base_workflow_file_exists", str(e)))
    
    # Test 4: Workflow has valid structure (basic check without YAML parser)
    try:
        ci_workflow = Path(".github/workflows/ci.yml")
        if ci_workflow.exists():
            with open(ci_workflow, 'r') as f:
                content = f.read()
                # Basic checks for required fields
                assert "name:" in content, "Workflow should have a name field"
                assert "on:" in content, "Workflow should have triggers"
                assert "jobs:" in content, "Workflow should have jobs"
                assert ("push:" in content or "pull_request:" in content), \
                    "Workflow should trigger on push or pull_request"
                passed_tests.append("test_workflow_structure")
        else:
            failed_tests.append(("test_workflow_structure", "Workflow file doesn't exist"))
    except AssertionError as e:
        failed_tests.append(("test_workflow_structure", str(e)))
    
    # Print results
    print(f"\n{'='*60}")
    print("GitHub Actions Structure Tests")
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
    
    # Return non-zero exit code if any tests failed
    return len(failed_tests)

if __name__ == "__main__":
    sys.exit(run_tests())