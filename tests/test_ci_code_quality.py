#!/usr/bin/env python3
"""Test CI code quality checks."""

import os
import sys
from pathlib import Path

def run_tests():
    """Run all tests for CI code quality checks."""
    failed_tests = []
    passed_tests = []
    
    ci_workflow = Path(".github/workflows/ci.yml")
    
    # Test 1: Code formatting check (clang-format)
    try:
        with open(ci_workflow, 'r') as f:
            content = f.read()
            
            assert "clang-format" in content or "format" in content.lower(), \
                "Should check code formatting"
            assert ".clang-format" in content or "style" in content.lower(), \
                "Should use clang-format configuration"
            
            passed_tests.append("test_code_formatting")
    except AssertionError as e:
        failed_tests.append(("test_code_formatting", str(e)))
    
    # Test 2: Static analysis (clang-tidy)
    try:
        with open(ci_workflow, 'r') as f:
            content = f.read()
            
            assert "clang-tidy" in content or "tidy" in content, \
                "Should run clang-tidy static analysis"
            assert ".clang-tidy" in content or "checks" in content.lower(), \
                "Should use clang-tidy configuration"
            
            passed_tests.append("test_static_analysis")
    except AssertionError as e:
        failed_tests.append(("test_static_analysis", str(e)))
    
    # Test 3: Additional static analysis (cppcheck)
    try:
        with open(ci_workflow, 'r') as f:
            content = f.read()
            
            assert "cppcheck" in content, \
                "Should run cppcheck for additional analysis"
            assert "--enable=" in content or "suppressions" in content, \
                "Should configure cppcheck properly"
            
            passed_tests.append("test_cppcheck_analysis")
    except AssertionError as e:
        failed_tests.append(("test_cppcheck_analysis", str(e)))
    
    # Test 4: Code quality job or steps
    try:
        with open(ci_workflow, 'r') as f:
            content = f.read()
            
            assert "code-quality" in content or "quality" in content or \
                   "lint" in content.lower(), \
                "Should have dedicated code quality checks"
            
            passed_tests.append("test_quality_job_exists")
    except AssertionError as e:
        failed_tests.append(("test_quality_job_exists", str(e)))
    
    # Test 5: Fail on quality issues
    try:
        with open(ci_workflow, 'r') as f:
            content = f.read()
            
            # Should not have continue-on-error for quality checks
            lines = content.split('\n')
            quality_section = False
            for i, line in enumerate(lines):
                if "clang-format" in line or "clang-tidy" in line or "cppcheck" in line:
                    quality_section = True
                if quality_section and "continue-on-error: true" in line:
                    raise AssertionError("Quality checks should fail the build")
            
            passed_tests.append("test_fail_on_quality_issues")
    except AssertionError as e:
        failed_tests.append(("test_fail_on_quality_issues", str(e)))
    
    # Print results
    print(f"\n{'='*60}")
    print("CI Code Quality Tests")
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