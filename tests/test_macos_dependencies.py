#!/usr/bin/env python3
"""Test macOS dependencies in CI workflow."""

import os
import sys
from pathlib import Path

def run_tests():
    """Run all tests for macOS dependencies."""
    failed_tests = []
    passed_tests = []
    
    ci_workflow = Path(".github/workflows/ci.yml")
    
    # Test 1: macOS dependencies include all required libraries
    try:
        with open(ci_workflow, 'r') as f:
            content = f.read()
            
            # Find macOS section
            macos_section = content[content.find("Install dependencies (macOS)"):]
            macos_section = macos_section[:macos_section.find("- name:")]
            
            # Check for required dependencies
            required_deps = ["cmake", "ccache"]
            for dep in required_deps:
                assert dep in macos_section, f"macOS should install {dep}"
            
            passed_tests.append("test_macos_basic_deps")
    except AssertionError as e:
        failed_tests.append(("test_macos_basic_deps", str(e)))
    except Exception as e:
        failed_tests.append(("test_macos_basic_deps", f"Error parsing workflow: {e}"))
    
    # Test 2: Check for raylib dependencies on macOS
    try:
        with open(ci_workflow, 'r') as f:
            content = f.read()
            
            macos_section = content[content.find("Install dependencies (macOS)"):]
            macos_section = macos_section[:macos_section.find("- name:")]
            
            # Raylib on macOS needs these
            raylib_deps = ["glfw", "openal-soft"]
            missing_deps = []
            for dep in raylib_deps:
                if dep not in macos_section:
                    missing_deps.append(dep)
            
            assert not missing_deps, f"Missing raylib dependencies: {', '.join(missing_deps)}"
            
            passed_tests.append("test_macos_raylib_deps")
    except AssertionError as e:
        failed_tests.append(("test_macos_raylib_deps", str(e)))
    
    # Test 3: Check for Homebrew formula flag
    try:
        with open(ci_workflow, 'r') as f:
            content = f.read()
            
            # Check if we're using --formula flag to avoid cask warnings
            assert "--formula" in content or "homebrew/cask" not in content, \
                "Should use --formula flag or avoid cask conflicts"
            
            passed_tests.append("test_homebrew_formula_flag")
    except AssertionError as e:
        failed_tests.append(("test_homebrew_formula_flag", str(e)))
    
    # Test 4: Check for error handling in macOS deps
    try:
        with open(ci_workflow, 'r') as f:
            content = f.read()
            
            macos_section = content[content.find("Install dependencies (macOS)"):]
            macos_section = macos_section[:macos_section.find("- name:")]
            
            # Should handle cases where deps are already installed
            assert "|| true" in macos_section or "brew upgrade" in macos_section, \
                "Should handle already installed dependencies"
            
            passed_tests.append("test_macos_error_handling")
    except AssertionError as e:
        failed_tests.append(("test_macos_error_handling", str(e)))
    
    # Print results
    print(f"\n{'='*60}")
    print("macOS Dependencies Tests")
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