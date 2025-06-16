#!/usr/bin/env python3
"""Test that all scripts use the correct executable name 'game_engine'"""

import os
import sys
import subprocess
import re

def test_full_test_fixed_uses_correct_executable():
    """Test that full_test_fixed.sh looks for 'game_engine' not 'game'"""
    print("Testing full_test_fixed.sh for correct executable name...")
    
    script_path = os.path.join(os.path.dirname(__file__), '..', 'full_test_fixed.sh')
    if not os.path.exists(script_path):
        print("✗ full_test_fixed.sh not found")
        return False
    
    with open(script_path, 'r') as f:
        content = f.read()
    
    # Check for references to old executable name 'game'
    # Exclude comments and help text
    problems = []
    lines = content.split('\n')
    for i, line in enumerate(lines, 1):
        # Skip comments and echo statements
        if line.strip().startswith('#') or 'echo' in line:
            continue
            
        # Look for problematic patterns
        if re.search(r'["\']game["\']\s*(]|$|\s|&&)', line):
            # Make sure it's not game_engine
            if 'game_engine' not in line:
                problems.append(f"Line {i}: {line.strip()}")
        elif re.search(r'\.\/game\s', line):
            problems.append(f"Line {i}: {line.strip()}")
        elif re.search(r'Executable\s+["\']game["\']', line):
            problems.append(f"Line {i}: {line.strip()}")
    
    if problems:
        print(f"✗ Found {len(problems)} references to old executable 'game':")
        for problem in problems[:5]:  # Show first 5
            print(f"  {problem}")
        if len(problems) > 5:
            print(f"  ... and {len(problems) - 5} more")
        return False
    
    print("✓ No references to old executable 'game' found")
    return True

def test_python_tests_use_correct_executable():
    """Test that Python test scripts use 'game_engine' not 'game'"""
    print("\nTesting Python test scripts for correct executable name...")
    
    test_dir = os.path.dirname(__file__)
    problems = []
    
    for filename in os.listdir(test_dir):
        if filename.endswith('.py') and filename.startswith('test_'):
            filepath = os.path.join(test_dir, filename)
            with open(filepath, 'r') as f:
                content = f.read()
                lines = content.split('\n')
                
            for i, line in enumerate(lines, 1):
                # Look for executable references
                if re.search(r'["\']\.\/game["\'](?!\w)', line) or \
                   re.search(r'executable.*=.*["\']game["\']', line, re.IGNORECASE):
                    if 'game_engine' not in line:
                        problems.append(f"{filename}:{i}: {line.strip()}")
    
    if problems:
        print(f"✗ Found {len(problems)} references to old executable 'game':")
        for problem in problems[:5]:
            print(f"  {problem}")
        if len(problems) > 5:
            print(f"  ... and {len(problems) - 5} more")
        return False
    
    print("✓ All Python tests use correct executable name")
    return True

def test_executable_exists():
    """Test that game_engine executable exists in build directory"""
    print("\nTesting for game_engine executable existence...")
    
    build_dir = os.path.join(os.path.dirname(__file__), '..', 'build')
    game_engine_path = os.path.join(build_dir, 'game_engine')
    
    if os.path.exists(game_engine_path) and os.access(game_engine_path, os.X_OK):
        print(f"✓ game_engine executable found at: {game_engine_path}")
        return True
    else:
        print(f"✗ game_engine executable not found or not executable at: {game_engine_path}")
        return False

def main():
    print("=== Executable Name Tests ===\n")
    
    tests = [
        test_full_test_fixed_uses_correct_executable,
        test_python_tests_use_correct_executable,
        test_executable_exists
    ]
    
    passed = sum(test() for test in tests)
    total = len(tests)
    
    print(f"\n=== Results: {passed}/{total} tests passed ===")
    
    return 0 if passed == total else 1

if __name__ == "__main__":
    sys.exit(main())