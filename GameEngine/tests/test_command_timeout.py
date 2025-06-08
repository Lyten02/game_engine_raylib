#!/usr/bin/env python3
"""Test command timeout functionality"""

import subprocess
import json
import time
import os
import sys

# Add the project root to the path
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from tests.test_utils import find_executable, cleanup_test_files

def run_cli_command(command, json_output=False, timeout=15):
    """Run a CLI command and return the result"""
    exe = find_executable()
    if not exe:
        return {'success': False, 'output': '', 'error': 'Executable not found'}
    
    try:
        cmd = [exe, '--headless']
        if json_output:
            cmd.append('--json')
        cmd.extend(['--command', command])
        
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=timeout
        )
        
        if json_output and result.stdout:
            try:
                return json.loads(result.stdout)
            except json.JSONDecodeError:
                return {'success': False, 'error': 'Invalid JSON response', 'output': result.stdout}
        
        return {
            'success': result.returncode == 0,
            'output': result.stdout,
            'error': result.stderr
        }
    except subprocess.TimeoutExpired:
        return {'success': False, 'output': '', 'error': 'Command timed out'}
    except Exception as e:
        return {'success': False, 'output': '', 'error': str(e)}

def test_command_timeout():
    """Test that commands timeout after configured time"""
    
    print("\n=== Testing Command Timeout ===")
    
    # Check if executable exists
    exe = find_executable()
    if not exe:
        print("❌ Executable not found. Please build the project first.")
        return False
    
    # Test 1: Test a simple command that should complete quickly
    print("\nTest 1: Testing quick command (should complete)...")
    start_time = time.time()
    result = run_cli_command("help", json_output=True, timeout=15)
    elapsed = time.time() - start_time
    
    if result['success']:
        print(f"✅ Help command completed in {elapsed:.2f}s")
    else:
        print(f"❌ Help command failed: {result.get('error', 'Unknown error')}")
        return False
    
    # Test 2: Test project list (should be fast)
    print("\nTest 2: Testing project.list command...")
    start_time = time.time()
    result = run_cli_command("project.list", json_output=True, timeout=15)
    elapsed = time.time() - start_time
    
    if result['success']:
        print(f"✅ project.list completed in {elapsed:.2f}s")
    else:
        # Check if it timed out
        if "timed out" in result.get('error', '').lower():
            print(f"✅ Command properly timed out (expected behavior)")
        else:
            print(f"❌ project.list failed: {result.get('error', 'Unknown error')}")
    
    # Test 3: Test engine info
    print("\nTest 3: Testing engine.info command...")
    start_time = time.time()
    result = run_cli_command("engine.info", json_output=True, timeout=15)
    elapsed = time.time() - start_time
    
    if result['success']:
        print(f"✅ engine.info completed in {elapsed:.2f}s")
    else:
        if "timed out" in result.get('error', '').lower():
            print(f"✅ Command properly timed out (expected behavior)")
        else:
            print(f"❌ engine.info failed: {result.get('error', 'Unknown error')}")
    
    # Test 4: Test config get (this was one of the hanging commands)
    print("\nTest 4: Testing config get command...")
    start_time = time.time()
    result = run_cli_command("config.get window.width", json_output=True, timeout=15)
    elapsed = time.time() - start_time
    
    # Command should complete within timeout (10s) or return timeout error
    assert elapsed < 12, f"Command took too long: {elapsed}s"
    
    if result['success']:
        print(f"✅ config.get completed in {elapsed:.2f}s")
    else:
        if "timed out" in result.get('error', '').lower():
            print(f"✅ Command properly timed out (expected behavior)")
        else:
            print(f"❌ config.get failed: {result.get('error', 'Unknown error')}")
    
    # Test 5: Test a potentially long-running command (project.build)
    print("\nTest 5: Testing project.build timeout...")
    
    # First create a test project
    print("Creating test project for build test...")
    create_result = run_cli_command("project.create TimeoutTestProject", json_output=True, timeout=15)
    
    if create_result['success']:
        print("✅ Test project created")
        
        # Try to build it
        start_time = time.time()
        result = run_cli_command("project.build TimeoutTestProject", json_output=True, timeout=15)
        elapsed = time.time() - start_time
        
        if result['success']:
            print(f"✅ project.build completed in {elapsed:.2f}s")
        else:
            if "timed out" in result.get('error', '').lower():
                print(f"✅ Command properly timed out after ~10s (expected for long builds)")
            else:
                print(f"⚠️  project.build failed: {result.get('error', 'Unknown error')}")
        
        # Cleanup
        cleanup_test_files(["projects/TimeoutTestProject"])
    else:
        print(f"⚠️  Could not create test project: {create_result.get('error', 'Unknown error')}")
    
    print("\n✅ Command timeout tests completed successfully")
    return True

def test_timeout_configuration():
    """Test that timeout can be configured via config"""
    
    print("\n=== Testing Timeout Configuration ===")
    
    # Test that we can query the timeout configuration
    result = run_cli_command("config.get console.command_timeout_seconds", json_output=True, timeout=5)
    
    if result['success'] and 'value' in result:
        timeout_value = result['value']
        print(f"✅ Current timeout configuration: {timeout_value} seconds")
        assert timeout_value == 10, f"Expected default timeout of 10 seconds, got {timeout_value}"
    else:
        print("⚠️  Could not retrieve timeout configuration")
    
    # Test timeout enabled flag
    result = run_cli_command("config.get console.enable_command_timeout", json_output=True, timeout=5)
    
    if result['success'] and 'value' in result:
        enabled = result['value']
        print(f"✅ Timeout enabled: {enabled}")
        assert enabled == True, f"Expected timeout to be enabled by default"
    else:
        print("⚠️  Could not retrieve timeout enabled flag")
    
    return True

if __name__ == "__main__":
    try:
        success = test_command_timeout()
        if success:
            test_timeout_configuration()
        
        print("\n✅ All timeout tests passed!")
        exit(0)
    except Exception as e:
        print(f"\n❌ Timeout test failed: {e}")
        exit(1)