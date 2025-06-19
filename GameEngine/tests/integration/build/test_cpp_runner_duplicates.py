#!/usr/bin/env python3
"""Test for duplicate definitions in cpp_test_runner.cpp"""

import os
import sys
import re

def test_no_duplicate_definitions():
    """Check that cpp_test_runner.cpp has no duplicate variable definitions"""
    print("Testing cpp_test_runner.cpp for duplicate definitions...")
    
    cpp_runner_path = os.path.join(os.path.dirname(__file__), 'cpp_test_runner.cpp')
    if not os.path.exists(cpp_runner_path):
        print("✗ cpp_test_runner.cpp not found")
        return False
    
    with open(cpp_runner_path, 'r') as f:
        lines = f.readlines()
    
    # Track variable definitions
    definitions = {}
    duplicates = []
    
    for i, line in enumerate(lines, 1):
        # Look for TestDefinition variable definitions
        match = re.match(r'\s*TestDefinition\s+(\w+)\s*\(', line)
        if match:
            var_name = match.group(1)
            if var_name in definitions:
                duplicates.append({
                    'variable': var_name,
                    'first_line': definitions[var_name],
                    'duplicate_line': i,
                    'first_content': lines[definitions[var_name] - 1].strip(),
                    'duplicate_content': line.strip()
                })
            else:
                definitions[var_name] = i
    
    if duplicates:
        print(f"✗ Found {len(duplicates)} duplicate variable definitions:")
        for dup in duplicates:
            print(f"\n  Variable '{dup['variable']}':")
            print(f"    First definition at line {dup['first_line']}:")
            print(f"      {dup['first_content']}")
            print(f"    Duplicate at line {dup['duplicate_line']}:")
            print(f"      {dup['duplicate_content']}")
        return False
    
    print(f"✓ No duplicate definitions found ({len(definitions)} unique definitions)")
    return True

def main():
    print("=== C++ Test Runner Duplicate Check ===\n")
    
    if test_no_duplicate_definitions():
        print("\n✓ All tests passed")
        return 0
    else:
        print("\n✗ Test failed")
        return 1

if __name__ == "__main__":
    sys.exit(main())