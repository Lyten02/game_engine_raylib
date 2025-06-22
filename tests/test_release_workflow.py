#!/usr/bin/env python3
"""Test release workflow configuration."""

import os
import sys
from pathlib import Path

def run_tests():
    """Run all tests for release workflow."""
    failed_tests = []
    passed_tests = []
    
    release_workflow = Path(".github/workflows/release.yml")
    
    # Test 1: Release workflow file exists
    try:
        assert release_workflow.exists(), "Release workflow file should exist"
        assert release_workflow.is_file(), "release.yml should be a file"
        passed_tests.append("test_release_workflow_exists")
    except AssertionError as e:
        failed_tests.append(("test_release_workflow_exists", str(e)))
    
    # Test 2: Workflow triggers on tags
    try:
        if release_workflow.exists():
            with open(release_workflow, 'r') as f:
                content = f.read()
                
                assert "on:" in content, "Should have triggers"
                assert "push:" in content, "Should trigger on push"
                assert "tags:" in content, "Should trigger on tags"
                assert "v*" in content or "v[0-9]" in content, \
                    "Should trigger on version tags (v*)"
                
                passed_tests.append("test_tag_trigger")
        else:
            failed_tests.append(("test_tag_trigger", "Release workflow doesn't exist"))
    except AssertionError as e:
        failed_tests.append(("test_tag_trigger", str(e)))
    
    # Test 3: Multi-platform builds
    try:
        if release_workflow.exists():
            with open(release_workflow, 'r') as f:
                content = f.read()
                
                assert "matrix:" in content, "Should have matrix strategy"
                assert "ubuntu-latest" in content, "Should build for Linux"
                assert "macos-latest" in content, "Should build for macOS"
                assert "windows-latest" in content, "Should build for Windows"
                
                passed_tests.append("test_multi_platform_builds")
        else:
            failed_tests.append(("test_multi_platform_builds", "Release workflow doesn't exist"))
    except AssertionError as e:
        failed_tests.append(("test_multi_platform_builds", str(e)))
    
    # Test 4: Release creation
    try:
        if release_workflow.exists():
            with open(release_workflow, 'r') as f:
                content = f.read()
                
                assert "release" in content.lower(), "Should create releases"
                assert "actions/create-release" in content or \
                       "softprops/action-gh-release" in content or \
                       "gh release" in content, \
                    "Should use release action"
                
                passed_tests.append("test_release_creation")
        else:
            failed_tests.append(("test_release_creation", "Release workflow doesn't exist"))
    except AssertionError as e:
        failed_tests.append(("test_release_creation", str(e)))
    
    # Test 5: Binary artifact upload
    try:
        if release_workflow.exists():
            with open(release_workflow, 'r') as f:
                content = f.read()
                
                assert "upload" in content.lower(), "Should upload artifacts"
                assert "game_engine" in content or "*.exe" in content or \
                       "*.app" in content or "binary" in content, \
                    "Should upload game engine binaries"
                assert "zip" in content or "tar" in content or "archive" in content, \
                    "Should create archives for release"
                
                passed_tests.append("test_artifact_upload")
        else:
            failed_tests.append(("test_artifact_upload", "Release workflow doesn't exist"))
    except AssertionError as e:
        failed_tests.append(("test_artifact_upload", str(e)))
    
    # Print results
    print(f"\n{'='*60}")
    print("Release Workflow Tests")
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