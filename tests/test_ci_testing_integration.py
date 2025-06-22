#!/usr/bin/env python3
"""Test CI testing integration in workflow."""

import os
import sys
from pathlib import Path

def run_tests():
    """Run all tests for CI testing integration."""
    failed_tests = []
    passed_tests = []
    
    ci_workflow = Path(".github/workflows/ci.yml")
    
    # Test 1: Workflow contains test job
    try:
        with open(ci_workflow, 'r') as f:
            content = f.read()
            
            # Check for test job or test steps in build job
            assert "test" in content.lower(), "Workflow should contain testing"
            assert "make test" in content or "ctest" in content or "pytest" in content, \
                "Should run tests using make test, ctest, or pytest"
            
            passed_tests.append("test_has_test_execution")
    except AssertionError as e:
        failed_tests.append(("test_has_test_execution", str(e)))
    
    # Test 2: Check for all test types
    try:
        with open(ci_workflow, 'r') as f:
            content = f.read()
            
            # Should run different test categories
            assert "test-unit" in content or "test-cpp" in content or "all-tests" in content, \
                "Should run unit tests"
            assert "test-integration" in content or "all-tests" in content, \
                "Should run integration tests"
            assert "test-system" in content or "all-tests" in content, \
                "Should run system tests"
            
            passed_tests.append("test_all_test_types")
    except AssertionError as e:
        failed_tests.append(("test_all_test_types", str(e)))
    
    # Test 3: Test result reporting
    try:
        with open(ci_workflow, 'r') as f:
            content = f.read()
            
            # Check for test result handling
            assert "test-results" in content or "test_results" in content or \
                   "junit" in content or "test-report" in content, \
                "Should handle test results/reports"
            
            passed_tests.append("test_result_reporting")
    except AssertionError as e:
        failed_tests.append(("test_result_reporting", str(e)))
    
    # Test 4: Test artifact upload
    try:
        with open(ci_workflow, 'r') as f:
            content = f.read()
            
            # Check for test artifact/log upload
            assert "test_log" in content or "logs/" in content or \
                   "test-artifacts" in content or "test_results.json" in content, \
                "Should upload test logs or artifacts"
            
            passed_tests.append("test_artifact_upload")
    except AssertionError as e:
        failed_tests.append(("test_artifact_upload", str(e)))
    
    # Test 5: Test failure handling
    try:
        with open(ci_workflow, 'r') as f:
            content = f.read()
            
            # Check for proper test step configuration
            assert "Run tests" in content or "Test" in content, \
                "Should have dedicated test step"
            assert "continue-on-error: false" in content or \
                   "if: success()" in content or \
                   "if-no-files-found: error" in content or \
                   not "continue-on-error: true" in content, \
                "Tests should fail the build on error"
            
            passed_tests.append("test_failure_handling")
    except AssertionError as e:
        failed_tests.append(("test_failure_handling", str(e)))
    
    # Print results
    print(f"\n{'='*60}")
    print("CI Testing Integration Tests")
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