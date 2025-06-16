#!/usr/bin/env python3
"""
Test to verify engine executable naming
"""

import os
import subprocess
import sys

def test_engine_executable_name():
    """Test that the engine executable has proper name"""
    print("Testing engine executable naming...")
    
    # Check if CMakeLists.txt contains correct executable name
    with open('CMakeLists.txt', 'r') as f:
        content = f.read()
        
    # Test 1: Check if add_executable uses game_engine instead of game
    if 'add_executable(game_engine' in content:
        print("✓ CMakeLists.txt uses 'game_engine' as executable name")
        return True
    elif 'add_executable(game' in content:
        print("✗ CMakeLists.txt still uses 'game' as executable name")
        return False
    else:
        print("✗ Could not find add_executable in CMakeLists.txt")
        return False

def test_build_scripts_reference():
    """Test that build scripts reference correct executable"""
    print("\nTesting build script references...")
    
    scripts_to_check = [
        'rebuild.sh',
        'rebuild_fast.sh', 
        'rebuild_incremental.sh',
        'rebuild_smart.sh'
    ]
    
    all_correct = True
    
    for script in scripts_to_check:
        if os.path.exists(script):
            with open(script, 'r') as f:
                content = f.read()
            
            if 'build/game_engine' in content and 'build/game_engine' not in content.replace('build/game_engine', ''):
                print(f"✓ {script} references 'game_engine'")
            else:
                print(f"✗ {script} still references 'game'")
                all_correct = False
    
    return all_correct

def test_gitignore():
    """Test that .gitignore includes new executable name"""
    print("\nTesting .gitignore...")
    
    with open('.gitignore', 'r') as f:
        content = f.read()
    
    if '/build/game_engine' in content or '/game_engine' in content:
        print("✓ .gitignore includes 'game_engine'")
        return True
    else:
        print("✗ .gitignore does not include 'game_engine'")
        return False

def main():
    """Run all tests"""
    print("=== Engine Naming Tests ===\n")
    
    # Change to project root if in tests directory
    if os.path.basename(os.getcwd()) == 'tests':
        os.chdir('..')
    
    tests_passed = 0
    tests_total = 3
    
    if test_engine_executable_name():
        tests_passed += 1
        
    if test_build_scripts_reference():
        tests_passed += 1
        
    if test_gitignore():
        tests_passed += 1
    
    print(f"\n=== Results: {tests_passed}/{tests_total} tests passed ===")
    
    return 0 if tests_passed == tests_total else 1

if __name__ == "__main__":
    sys.exit(main())