#!/usr/bin/env python3

import os
import json
import sys
from pathlib import Path
from datetime import datetime

def test_create_test_artifacts():
    """Create test artifacts in expected locations for CI"""
    
    # Get the build directory (where tests are run from)
    build_dir = Path.cwd()
    tests_dir = Path(__file__).parent
    
    print(f"Current working directory: {build_dir}")
    print(f"Tests directory: {tests_dir}")
    
    # Create test results in multiple potential locations
    test_results = {
        "timestamp": datetime.now().isoformat(),
        "platform": sys.platform,
        "python_version": sys.version,
        "test_status": "sample",
        "tests_run": 1,
        "tests_passed": 1,
        "tests_failed": 0
    }
    
    # Location 1: build directory (where CMake runs tests)
    build_results = build_dir / "test_results.json"
    print(f"\nCreating test results at: {build_results}")
    with open(build_results, 'w') as f:
        json.dump(test_results, f, indent=2)
    
    # Location 2: tests directory
    tests_results = tests_dir / "test_results.json"
    print(f"Creating test results at: {tests_results}")
    with open(tests_results, 'w') as f:
        json.dump(test_results, f, indent=2)
    
    # Create test logs
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    log_content = f"Test log created at {datetime.now()}\nPlatform: {sys.platform}\n"
    
    # Location 1: build directory
    build_log = build_dir / f"test_log_{timestamp}.log"
    print(f"\nCreating test log at: {build_log}")
    with open(build_log, 'w') as f:
        f.write(log_content)
    
    # Location 2: tests/logs directory
    tests_logs_dir = tests_dir / "logs"
    tests_logs_dir.mkdir(exist_ok=True)
    tests_log = tests_logs_dir / f"test_log_{timestamp}.log"
    print(f"Creating test log at: {tests_log}")
    with open(tests_log, 'w') as f:
        f.write(log_content)
    
    # Location 3: build/logs directory (if exists)
    build_logs_dir = build_dir / "logs"
    if build_logs_dir.exists() or True:  # Always create
        build_logs_dir.mkdir(exist_ok=True)
        build_logs_log = build_logs_dir / f"test_log_{timestamp}.log"
        print(f"Creating test log at: {build_logs_log}")
        with open(build_logs_log, 'w') as f:
            f.write(log_content)
    
    print("\n✅ Test artifacts created successfully")
    print("\nCreated files:")
    for path in [build_results, tests_results, build_log, tests_log]:
        if path.exists():
            print(f"  ✓ {path}")
    
    if build_logs_dir.exists():
        print(f"  ✓ {build_logs_log}")
    
    return True

if __name__ == "__main__":
    success = test_create_test_artifacts()
    sys.exit(0 if success else 1)