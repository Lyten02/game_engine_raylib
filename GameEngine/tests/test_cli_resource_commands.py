#!/usr/bin/env python3
"""
Test CLI resource commands for stability - no timeouts should occur
"""

import sys
import os
import subprocess
import time
import json

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from test_utils import run_cli_command, build_engine_if_needed, find_executable, print_test_header, print_test_result

def test_resource_list_command():
    """Test resource list command doesn't timeout"""
    print_test_header("Resource List Command")
    
    start_time = time.time()
    result = run_cli_command(['resource.list'], timeout=5)
    elapsed = time.time() - start_time
    
    success = result['error'] != 'Command timed out' and elapsed < 2
    print_test_result("resource list completes quickly", success)
    
    if not success:
        print(f"  Elapsed: {elapsed:.2f}s")
        print(f"  Error: {result['error']}")
    
    return success

def test_config_commands_with_resources():
    """Test config commands that might trigger resource initialization"""
    print_test_header("Config Commands")
    
    config_keys = [
        'window.width',
        'window.height', 
        'window.title',
        'console.font_size',
        'scripting.lua_enabled'
    ]
    
    all_passed = True
    
    for key in config_keys:
        start_time = time.time()
        result = run_cli_command(['config.get', key], timeout=5)
        elapsed = time.time() - start_time
        
        success = result['success'] and elapsed < 1
        print_test_result(f"config get {key}", success)
        
        if not success:
            print(f"  Elapsed: {elapsed:.2f}s")
            if result['error']:
                print(f"  Error: {result['error']}")
            all_passed = False
    
    return all_passed

def test_entity_commands():
    """Test entity commands that might interact with resources"""
    print_test_header("Entity Commands")
    
    # Create a test entity
    start_time = time.time()
    result = run_cli_command(['entity.create', 'TestEntity'], timeout=5)
    elapsed = time.time() - start_time
    
    create_success = result['success'] and elapsed < 1
    print_test_result("entity create completes quickly", create_success)
    
    # List entities
    start_time = time.time()
    result = run_cli_command(['entity.list'], timeout=5)
    elapsed = time.time() - start_time
    
    list_success = result['success'] and elapsed < 1
    print_test_result("entity list completes quickly", list_success)
    
    return create_success and list_success

def test_scene_commands():
    """Test scene commands that might trigger resource loading"""
    print_test_header("Scene Commands")
    
    # Create scene
    start_time = time.time()
    result = run_cli_command(['scene.create', 'test_cli_scene'], timeout=5)
    elapsed = time.time() - start_time
    
    create_success = result['success'] and elapsed < 1
    print_test_result("scene create completes quickly", create_success)
    
    # List scenes
    start_time = time.time()
    result = run_cli_command(['scene.list'], timeout=5)
    elapsed = time.time() - start_time
    
    list_success = result['success'] and elapsed < 1
    print_test_result("scene list completes quickly", list_success)
    
    # Save scene
    start_time = time.time()
    result = run_cli_command(['scene.save'], timeout=5)
    elapsed = time.time() - start_time
    
    save_success = elapsed < 2  # Save might take slightly longer
    print_test_result("scene save completes without hanging", save_success)
    
    return create_success and list_success and save_success

def test_json_mode_commands():
    """Test commands in JSON mode"""
    print_test_header("JSON Mode Commands")
    
    exe = find_executable()
    if not exe:
        print("ERROR: Could not find executable")
        return False
    
    # Test config get in JSON mode
    start_time = time.time()
    result = subprocess.run(
        [exe, '--json', '--headless', '--command', 'config.get window.width'],
        capture_output=True,
        text=True,
        timeout=5
    )
    elapsed = time.time() - start_time
    
    json_success = False
    if result.returncode == 0 and elapsed < 2:
        try:
            json_data = json.loads(result.stdout)
            json_success = json_data.get('success', False)
        except:
            pass
    
    print_test_result("JSON mode config command", json_success)
    
    # Test entity list in JSON mode
    start_time = time.time()
    result = subprocess.run(
        [exe, '--json', '--headless', '--command', 'entity.list'],
        capture_output=True,
        text=True,
        timeout=5
    )
    elapsed = time.time() - start_time
    
    json_list_success = False
    if result.returncode == 0 and elapsed < 2:
        try:
            json_data = json.loads(result.stdout)
            json_list_success = 'entities' in json_data or json_data.get('success', False)
        except:
            pass
    
    print_test_result("JSON mode entity list command", json_list_success)
    
    return json_success and json_list_success

def test_rapid_commands():
    """Test rapid command execution doesn't cause issues"""
    print_test_header("Rapid Command Execution")
    
    exe = find_executable()
    if not exe:
        print("ERROR: Could not find executable")
        return False
    
    # Execute many commands rapidly
    start_time = time.time()
    success_count = 0
    total_commands = 20
    
    for i in range(total_commands):
        result = subprocess.run(
            [exe, '--cli', 'config.get', 'window.width'],
            capture_output=True,
            text=True,
            timeout=2
        )
        if result.returncode == 0:
            success_count += 1
    
    elapsed = time.time() - start_time
    
    all_success = success_count == total_commands
    time_ok = elapsed < 10  # Should complete 20 commands in less than 10 seconds
    
    print_test_result(f"All {total_commands} rapid commands succeeded", all_success)
    print_test_result(f"Rapid execution completed in reasonable time ({elapsed:.1f}s)", time_ok)
    
    return all_success and time_ok

def main():
    print("=" * 60)
    print("CLI Resource Commands Stability Test")
    print("=" * 60)
    
    # Build engine if needed
    build_engine_if_needed()
    
    # Find the executable
    exe = find_executable()
    if not exe:
        print("ERROR: Could not find game executable")
        return False
    
    # Run all tests
    tests_passed = 0
    total_tests = 6
    
    if test_resource_list_command():
        tests_passed += 1
    
    if test_config_commands_with_resources():
        tests_passed += 1
    
    if test_entity_commands():
        tests_passed += 1
    
    if test_scene_commands():
        tests_passed += 1
    
    if test_json_mode_commands():
        tests_passed += 1
    
    if test_rapid_commands():
        tests_passed += 1
    
    # Summary
    print("\n" + "=" * 60)
    print(f"Test Summary: {tests_passed}/{total_tests} passed")
    print("=" * 60)
    
    return tests_passed == total_tests

if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)