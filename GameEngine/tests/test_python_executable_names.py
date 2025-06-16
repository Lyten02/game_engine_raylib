#!/usr/bin/env python3
"""Test that Python test files use correct executable name"""

import os
import sys
import re

def find_executable_references():
    """Find all references to executable in Python test files"""
    test_dir = os.path.dirname(__file__)
    issues = []
    
    # Patterns to find executable references
    patterns = [
        # Direct executable references
        (r'["\']\.?/?game["\'](?!\w)', 'Direct reference to "game"'),
        (r'executable\s*=\s*["\']\.?/?game["\']', 'executable = "game"'),
        (r'GAME_EXECUTABLE\s*=\s*["\'].*?/?game["\']', 'GAME_EXECUTABLE = "game"'),
        # Checking for game file
        (r'exists\(["\']game["\']\)', 'exists("game")'),
        (r'exists\(["\']\.\/game["\']\)', 'exists("./game")'),
        # Build paths
        (r'build/game["\']', 'build/game path'),
        (r'/game\s+', 'Command with /game'),
    ]
    
    for filename in os.listdir(test_dir):
        if filename.endswith('.py') and filename != os.path.basename(__file__):
            filepath = os.path.join(test_dir, filename)
            with open(filepath, 'r') as f:
                lines = f.readlines()
            
            for i, line in enumerate(lines, 1):
                # Skip comments and string literals in print/log statements
                if line.strip().startswith('#'):
                    continue
                    
                # Skip if it's clearly referring to game_engine
                if 'game_engine' in line:
                    continue
                    
                for pattern, description in patterns:
                    if re.search(pattern, line):
                        # Additional check to avoid false positives
                        if 'game_engine' not in line and 'game engine' not in line.lower():
                            issues.append({
                                'file': filename,
                                'line': i,
                                'content': line.strip(),
                                'issue': description
                            })
    
    return issues

def main():
    print("=== Python Test Executable Name Check ===\n")
    
    issues = find_executable_references()
    
    if not issues:
        print("✓ All Python tests use correct executable name 'game_engine'")
        return 0
    
    print(f"✗ Found {len(issues)} references to old executable name 'game':\n")
    
    # Group by file
    by_file = {}
    for issue in issues:
        if issue['file'] not in by_file:
            by_file[issue['file']] = []
        by_file[issue['file']].append(issue)
    
    for filename, file_issues in by_file.items():
        print(f"\n{filename}:")
        for issue in file_issues:
            print(f"  Line {issue['line']}: {issue['issue']}")
            print(f"    {issue['content']}")
    
    print(f"\n✗ Total issues: {len(issues)} in {len(by_file)} files")
    return 1

if __name__ == "__main__":
    sys.exit(main())