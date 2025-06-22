#!/usr/bin/env python3
"""Test CI build workflow configuration."""

import os
import sys
from pathlib import Path

def run_tests():
    """Run all tests for CI build workflow."""
    failed_tests = []
    passed_tests = []
    
    ci_workflow = Path(".github/workflows/ci.yml")
    
    # Test 1: Workflow contains build job with proper configuration
    try:
        with open(ci_workflow, 'r') as f:
            content = f.read()
            
            # Check for build job
            assert "build:" in content, "Workflow should have a build job"
            
            # Check for matrix strategy (multi-platform)
            assert "strategy:" in content, "Build job should have strategy"
            assert "matrix:" in content, "Build job should have matrix for multi-platform builds"
            assert "os:" in content, "Matrix should define OS targets"
            
            # Check for Ubuntu, macOS, Windows support
            assert "ubuntu-latest" in content, "Should build on Ubuntu"
            assert "macos-latest" in content, "Should build on macOS"
            assert "windows-latest" in content, "Should build on Windows"
            
            passed_tests.append("test_multi_platform_support")
    except AssertionError as e:
        failed_tests.append(("test_multi_platform_support", str(e)))
    
    # Test 2: Check for proper build steps
    try:
        with open(ci_workflow, 'r') as f:
            content = f.read()
            
            # Essential build steps
            assert "actions/checkout@" in content, "Should checkout code"
            assert "cmake" in content.lower(), "Should use CMake for building"
            assert "make" in content or "cmake --build" in content, "Should have build command"
            
            passed_tests.append("test_build_steps")
    except AssertionError as e:
        failed_tests.append(("test_build_steps", str(e)))
    
    # Test 3: Check for dependency caching
    try:
        with open(ci_workflow, 'r') as f:
            content = f.read()
            
            # Check for cache action
            assert "actions/cache@" in content or "cache:" in content, \
                "Should use caching for dependencies"
            assert "ccache" in content or "cache" in content.lower(), \
                "Should have cache configuration"
            
            passed_tests.append("test_dependency_caching")
    except AssertionError as e:
        failed_tests.append(("test_dependency_caching", str(e)))
    
    # Test 4: Check for proper dependency installation
    try:
        with open(ci_workflow, 'r') as f:
            content = f.read()
            
            # Check for dependency installation steps
            assert "Install dependencies" in content or "install" in content.lower(), \
                "Should have dependency installation step"
            
            # Platform-specific checks
            assert "apt-get" in content or "apt" in content or "brew" in content or "choco" in content, \
                "Should install system dependencies per platform"
            
            passed_tests.append("test_dependency_installation")
    except AssertionError as e:
        failed_tests.append(("test_dependency_installation", str(e)))
    
    # Test 5: Check for artifact upload
    try:
        with open(ci_workflow, 'r') as f:
            content = f.read()
            
            assert "actions/upload-artifact@" in content, \
                "Should upload build artifacts"
            assert "game_engine" in content or "artifact" in content, \
                "Should specify artifact name"
            
            passed_tests.append("test_artifact_upload")
    except AssertionError as e:
        failed_tests.append(("test_artifact_upload", str(e)))
    
    # Print results
    print(f"\n{'='*60}")
    print("CI Build Workflow Tests")
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