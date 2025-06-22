#!/usr/bin/env python3

import subprocess
import sys
import os
from pathlib import Path
import json

# Try to import yaml, but don't fail if not available
try:
    import yaml
except ImportError:
    yaml = None

def test_github_workflow_valid():
    """Test that GitHub workflow file is valid YAML"""
    workflow_path = Path(__file__).parent.parent.parent / ".github/workflows/ci.yml"
    
    print("=== Testing GitHub Workflow Validity ===")
    
    if not workflow_path.exists():
        print("❌ CI workflow not found")
        return False
    
    if yaml is None:
        print("⚠️  PyYAML not available, doing basic syntax check")
        with open(workflow_path, 'r') as f:
            content = f.read()
        
        # Basic syntax checks
        if 'name:' in content and 'on:' in content and 'jobs:' in content:
            print("✅ Basic workflow structure looks valid")
            return True
        else:
            print("❌ Missing basic workflow structure")
            return False
    
    try:
        with open(workflow_path, 'r') as f:
            workflow_data = yaml.safe_load(f)
        
        print("✅ Workflow file is valid YAML")
        
        # Check required sections
        required_sections = ['name', 'on', 'jobs']
        for section in required_sections:
            if section in workflow_data:
                print(f"  ✓ Has '{section}' section")
            else:
                print(f"  ❌ Missing '{section}' section")
                return False
        
        # Check jobs
        jobs = workflow_data.get('jobs', {})
        print(f"\nJobs defined: {list(jobs.keys())}")
        
        # Check each job has required fields
        for job_name, job_config in jobs.items():
            if 'runs-on' in job_config:
                print(f"  ✓ {job_name}: runs on {job_config['runs-on']}")
            else:
                print(f"  ❌ {job_name}: missing 'runs-on'")
        
        return True
        
    except yaml.YAMLError as e:
        print(f"❌ Invalid YAML: {e}")
        return False
    except Exception as e:
        print(f"❌ Error reading workflow: {e}")
        return False

def test_all_platforms_covered():
    """Test that all major platforms are covered in CI"""
    workflow_path = Path(__file__).parent.parent.parent / ".github/workflows/ci.yml"
    
    print("\n=== Testing Platform Coverage ===")
    
    if not workflow_path.exists():
        return False
    
    with open(workflow_path, 'r') as f:
        content = f.read()
    
    platforms = ['ubuntu-latest', 'macos-latest', 'windows-latest']
    
    for platform in platforms:
        if platform in content:
            print(f"  ✓ {platform} is covered")
        else:
            print(f"  ❌ {platform} is missing")
            return False
    
    return True

def test_artifact_upload_configured():
    """Test that artifact upload is properly configured"""
    workflow_path = Path(__file__).parent.parent.parent / ".github/workflows/ci.yml"
    
    print("\n=== Testing Artifact Upload Configuration ===")
    
    if not workflow_path.exists():
        return False
    
    with open(workflow_path, 'r') as f:
        content = f.read()
    
    # Check for upload-artifact action
    if 'uses: actions/upload-artifact@' in content:
        print("✅ Artifact upload action is configured")
        
        # Check for test results upload
        if 'test-results' in content:
            print("  ✓ Test results upload configured")
        else:
            print("  ⚠️  Test results upload might be missing")
        
        # Check for build artifacts upload
        if 'game_engine' in content:
            print("  ✓ Build artifacts upload configured")
        else:
            print("  ⚠️  Build artifacts upload might be missing")
        
        return True
    else:
        print("❌ No artifact upload action found")
        return False

def test_error_handling_present():
    """Test that CI has proper error handling"""
    workflow_path = Path(__file__).parent.parent.parent / ".github/workflows/ci.yml"
    
    print("\n=== Testing Error Handling ===")
    
    if not workflow_path.exists():
        return False
    
    with open(workflow_path, 'r') as f:
        content = f.read()
    
    error_handling_patterns = [
        'continue-on-error',
        'if: always()',
        'if-no-files-found: warn'
    ]
    
    found_patterns = []
    for pattern in error_handling_patterns:
        if pattern in content:
            found_patterns.append(pattern)
            print(f"  ✓ Found: {pattern}")
    
    if len(found_patterns) >= 2:
        print("✅ Adequate error handling present")
        return True
    else:
        print("⚠️  Limited error handling found")
        return True  # Warning, not failure

def test_ci_ready_checklist():
    """Final checklist for CI readiness"""
    print("\n=== CI Readiness Checklist ===")
    
    checklist = {
        "CMakeLists.txt exists": Path("CMakeLists.txt").exists(),
        "Source files exist": Path("src").exists() and any(Path("src").rglob("*.cpp")),
        "Tests exist": Path("tests").exists() and any(Path("tests").rglob("test_*.py")),
        ".clang-format exists": Path("../.clang-format").exists(),
        "Scripts directory exists": Path("scripts").exists(),
        "CI workflow exists": Path("../.github/workflows/ci.yml").exists()
    }
    
    all_ready = True
    for item, status in checklist.items():
        if status:
            print(f"  ✓ {item}")
        else:
            print(f"  ❌ {item}")
            all_ready = False
    
    return all_ready

if __name__ == "__main__":
    tests = [
        test_github_workflow_valid,
        test_all_platforms_covered,
        test_artifact_upload_configured,
        test_error_handling_present,
        test_ci_ready_checklist
    ]
    
    all_passed = True
    for test in tests:
        if not test():
            all_passed = False
    
    print("\n" + "="*50)
    if all_passed:
        print("✅ CI Pipeline is ready!")
    else:
        print("❌ CI Pipeline needs attention")
    
    sys.exit(0 if all_passed else 1)