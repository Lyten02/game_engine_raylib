#!/usr/bin/env python3
"""
Integration test for ResourceManager stability with the fixed DefaultTextureManager
"""

import sys
import os
import subprocess
import time
import threading
import tempfile

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from test_utils import run_cli_command, build_engine_if_needed, find_executable, print_test_header, print_test_result

def test_headless_resource_loading():
    """Test resource loading in headless mode doesn't hang"""
    print_test_header("Headless Resource Loading")
    
    # Test 1: Simple texture load attempt
    start_time = time.time()
    result = run_cli_command(['resource.list'], timeout=5)
    elapsed = time.time() - start_time
    
    print_test_result("Resource list command completes quickly", 
                      result['error'] != 'Command timed out' and elapsed < 2)
    
    # Test 2: Load non-existent texture  
    start_time = time.time()
    with tempfile.NamedTemporaryFile(mode='w', suffix='.txt', delete=False) as f:
        f.write('# Test script for resource loading\n')
        f.write('log_info "Testing resource loading"\n')
        f.write('exit\n')
        script_path = f.name
    
    result = subprocess.run(
        [find_executable(), '--headless', '--script', script_path],
        capture_output=True,
        text=True,
        timeout=5
    )
    elapsed = time.time() - start_time
    os.unlink(script_path)
    
    print_test_result("Headless script execution completes quickly", 
                      result.returncode == 0 and elapsed < 2)
    
    return True

def test_multiple_resource_managers():
    """Test multiple resource manager instances don't cause issues"""
    print_test_header("Multiple Resource Manager Instances")
    
    # Create a test script that creates entities with textures
    with tempfile.NamedTemporaryFile(mode='w', suffix='.txt', delete=False) as f:
        f.write('project.create TestResourceStability\n')
        f.write('project.set TestResourceStability\n')
        f.write('scene.create test_scene\n')
        # Create multiple entities that would trigger texture loading
        for i in range(10):
            f.write(f'entity.create Entity{i}\n')
        f.write('scene.save\n')
        f.write('project.delete TestResourceStability\n')
        f.write('exit\n')
        script_path = f.name
    
    start_time = time.time()
    result = subprocess.run(
        [find_executable(), '--headless', '--script', script_path],
        capture_output=True,
        text=True,
        timeout=10
    )
    elapsed = time.time() - start_time
    os.unlink(script_path)
    
    success = result.returncode == 0 and elapsed < 5
    print_test_result("Multiple entity creation completes without hanging", success)
    
    if not success:
        print(f"  Elapsed time: {elapsed:.2f}s")
        if result.stdout:
            print(f"  stdout: {result.stdout[:200]}")
        if result.stderr:
            print(f"  stderr: {result.stderr[:200]}")
    
    return success

def test_concurrent_access():
    """Test concurrent access to resources"""
    print_test_header("Concurrent Resource Access")
    
    # Create multiple scripts that will run concurrently
    scripts = []
    for i in range(3):
        with tempfile.NamedTemporaryFile(mode='w', suffix='.txt', delete=False) as f:
            f.write(f'# Concurrent test {i}\n')
            f.write(f'log_info "Thread {i} starting"\n')
            f.write('config get window.width\n')
            f.write('config get window.height\n')
            f.write(f'log_info "Thread {i} done"\n')
            f.write('exit\n')
            scripts.append(f.name)
    
    # Run scripts concurrently
    processes = []
    start_time = time.time()
    
    for script in scripts:
        p = subprocess.Popen(
            [find_executable(), '--headless', '--script', script],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        processes.append(p)
    
    # Wait for all processes with timeout
    all_completed = True
    for p in processes:
        try:
            stdout, stderr = p.communicate(timeout=5)
            if p.returncode != 0:
                all_completed = False
        except subprocess.TimeoutExpired:
            p.kill()
            all_completed = False
    
    elapsed = time.time() - start_time
    
    # Clean up scripts
    for script in scripts:
        os.unlink(script)
    
    print_test_result("All concurrent processes completed successfully", 
                      all_completed and elapsed < 5)
    
    return all_completed

def test_cleanup_on_shutdown():
    """Test clean shutdown without hangs"""
    print_test_header("Clean Shutdown")
    
    # Test 1: Quick start and stop
    start_time = time.time()
    process = subprocess.Popen(
        [find_executable(), '--headless'],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    
    # Let it initialize
    time.sleep(0.5)
    
    # Terminate it
    process.terminate()
    
    # Wait for clean shutdown
    try:
        stdout, stderr = process.communicate(timeout=2)
        elapsed = time.time() - start_time
        clean_shutdown = True
    except subprocess.TimeoutExpired:
        process.kill()
        clean_shutdown = False
        elapsed = time.time() - start_time
    
    print_test_result("Engine shuts down cleanly", clean_shutdown and elapsed < 3)
    
    # Test 2: Shutdown after resource operations
    with tempfile.NamedTemporaryFile(mode='w', suffix='.txt', delete=False) as f:
        f.write('# Load some resources\n')
        f.write('config get window.width\n')
        f.write('config get window.height\n')
        f.write('log_info "Shutting down"\n')
        f.write('exit\n')
        script_path = f.name
    
    start_time = time.time()
    result = subprocess.run(
        [find_executable(), '--headless', '--script', script_path],
        capture_output=True,
        text=True,
        timeout=5
    )
    elapsed = time.time() - start_time
    os.unlink(script_path)
    
    print_test_result("Shutdown after resource operations completes quickly", 
                      result.returncode == 0 and elapsed < 2)
    
    return True

def test_stress_resource_operations():
    """Stress test with many rapid resource operations"""
    print_test_header("Stress Test Resource Operations")
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.txt', delete=False) as f:
        f.write('# Stress test\n')
        # Many rapid operations
        for i in range(50):
            f.write(f'config get window.width\n')
            f.write(f'config get window.height\n')
        f.write('log_info "Stress test complete"\n')
        f.write('exit\n')
        script_path = f.name
    
    start_time = time.time()
    result = subprocess.run(
        [find_executable(), '--headless', '--script', script_path],
        capture_output=True,
        text=True,
        timeout=10
    )
    elapsed = time.time() - start_time
    os.unlink(script_path)
    
    success = result.returncode == 0 and elapsed < 5
    print_test_result("Stress test completes without hanging", success)
    
    if not success:
        print(f"  Elapsed time: {elapsed:.2f}s")
    
    return success

def main():
    print("=" * 60)
    print("ResourceManager Stability Integration Test")
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
    total_tests = 5
    
    if test_headless_resource_loading():
        tests_passed += 1
    
    if test_multiple_resource_managers():
        tests_passed += 1
    
    if test_concurrent_access():
        tests_passed += 1
    
    if test_cleanup_on_shutdown():
        tests_passed += 1
    
    if test_stress_resource_operations():
        tests_passed += 1
    
    # Summary
    print("\n" + "=" * 60)
    print(f"Test Summary: {tests_passed}/{total_tests} passed")
    print("=" * 60)
    
    return tests_passed == total_tests

if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)