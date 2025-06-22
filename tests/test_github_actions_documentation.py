#!/usr/bin/env python3
"""Test GitHub Actions documentation."""

import os
import sys
from pathlib import Path

def run_tests():
    """Run all tests for GitHub Actions documentation."""
    failed_tests = []
    passed_tests = []
    
    readme = Path("README.md")
    ci_docs = Path(".github/workflows/README.md")
    
    # Test 1: CI status badges in main README
    try:
        if readme.exists():
            with open(readme, 'r') as f:
                content = f.read()
                
                assert "github.com" in content and "actions" in content and "badge" in content, \
                    "Should have GitHub Actions status badge"
                assert "workflow" in content or "CI" in content, \
                    "Should reference CI workflow"
                
                passed_tests.append("test_ci_badges")
        else:
            failed_tests.append(("test_ci_badges", "README.md doesn't exist"))
    except AssertionError as e:
        failed_tests.append(("test_ci_badges", str(e)))
    
    # Test 2: Workflow documentation exists
    try:
        assert ci_docs.exists(), "Workflow README should exist"
        assert ci_docs.is_file(), "Workflow README should be a file"
        passed_tests.append("test_workflow_docs_exists")
    except AssertionError as e:
        failed_tests.append(("test_workflow_docs_exists", str(e)))
    
    # Test 3: Workflow documentation content
    try:
        if ci_docs.exists():
            with open(ci_docs, 'r') as f:
                content = f.read()
                
                # Check for essential sections
                assert "## Workflows" in content or "# Workflows" in content, \
                    "Should have workflows section"
                assert "ci.yml" in content, "Should document CI workflow"
                assert "release.yml" in content, "Should document release workflow"
                assert "## Usage" in content or "# Usage" in content, \
                    "Should have usage instructions"
                
                passed_tests.append("test_workflow_docs_content")
        else:
            failed_tests.append(("test_workflow_docs_content", "Workflow docs don't exist"))
    except AssertionError as e:
        failed_tests.append(("test_workflow_docs_content", str(e)))
    
    # Test 4: Local testing instructions
    try:
        if ci_docs.exists():
            with open(ci_docs, 'r') as f:
                content = f.read()
                
                assert "local" in content.lower() or "test" in content.lower(), \
                    "Should have local testing instructions"
                assert "act" in content or "docker" in content or "locally" in content, \
                    "Should mention tools for local testing"
                
                passed_tests.append("test_local_testing_docs")
        else:
            failed_tests.append(("test_local_testing_docs", "Workflow docs don't exist"))
    except AssertionError as e:
        failed_tests.append(("test_local_testing_docs", str(e)))
    
    # Test 5: Troubleshooting section
    try:
        if ci_docs.exists():
            with open(ci_docs, 'r') as f:
                content = f.read()
                
                assert "troubleshoot" in content.lower() or "debug" in content.lower() or \
                       "common issues" in content.lower(), \
                    "Should have troubleshooting section"
                
                passed_tests.append("test_troubleshooting_docs")
        else:
            failed_tests.append(("test_troubleshooting_docs", "Workflow docs don't exist"))
    except AssertionError as e:
        failed_tests.append(("test_troubleshooting_docs", str(e)))
    
    # Print results
    print(f"\n{'='*60}")
    print("GitHub Actions Documentation Tests")
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