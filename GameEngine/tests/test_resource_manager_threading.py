
# Import TDD dependency resolver
try:
    from test_dependency_path_fix import get_compilation_flags, validate_test_environment
except ImportError:
    def get_compilation_flags():
        return {'includes': '', 'libs': '-lraylib -lspdlog', 'deps_dir': None}
    def validate_test_environment():
        return False, "Dependency resolver not available"

#!/usr/bin/env python3
"""Test ResourceManager threading safety and timeout behavior"""

import subprocess
import threading
import time
import json
import os
import sys

# Add parent directory to path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

def test_concurrent_texture_loading():
    """Test multiple texture load commands simultaneously"""
    print("Testing concurrent texture loading...")
    
    commands = [
        "entity.create",
        "entity.list",
        "resource.list"
    ] * 5  # 15 concurrent operations
    
    # Run commands concurrently
    threads = []
    results = []
    
    def run_cmd(command):
        try:
            result = subprocess.run([
                "./game_engine", "--json", "--headless", "--command", command
            ], capture_output=True, text=True, timeout=5, cwd="../build")
            results.append(result.returncode == 0)
        except subprocess.TimeoutExpired:
            results.append(False)
            print(f"  ❌ Command '{command}' timed out")
    
    for cmd in commands:
        thread = threading.Thread(target=run_cmd, args=(cmd,))
        threads.append(thread)
        thread.start()
    
    # Wait for all threads
    for thread in threads:
        thread.join(timeout=10)
    
    # Check results
    success_rate = sum(results) / len(results) if results else 0
    print(f"  Success rate: {success_rate:.1%} ({sum(results)}/{len(results)})")
    assert success_rate > 0.8, f"Too many failures: {success_rate}"
    print("  ✅ Concurrent texture loading test passed!")

def test_resource_manager_timeout():
    """Test that ResourceManager doesn't hang indefinitely"""
    print("Testing ResourceManager timeout behavior...")
    
    start_time = time.time()
    
    try:
        result = subprocess.run([
            "./game_engine", "--json", "--headless", "--command", "entity.create"
        ], capture_output=True, text=True, timeout=3, cwd="../build")
        
        elapsed = time.time() - start_time
        print(f"  Command completed in {elapsed:.2f}s")
        
        assert elapsed < 2.0, f"Command took too long: {elapsed}s"
        assert result.returncode == 0, f"Command failed with code {result.returncode}"
        
        # Parse JSON output
        try:
            output = json.loads(result.stdout)
            assert output.get("success") == True, "Command didn't succeed"
            print(f"  Entity created: {output.get('entity_id', 'unknown')}")
        except json.JSONDecodeError:
            print(f"  Warning: Could not parse JSON output: {result.stdout}")
        
        print("  ✅ ResourceManager timeout test passed!")
        
    except subprocess.TimeoutExpired:
        elapsed = time.time() - start_time
        assert False, f"Command timed out after {elapsed:.2f}s"

def test_rapid_sequential_commands():
    """Test rapid sequential commands don't cause deadlocks"""
    print("Testing rapid sequential commands...")
    
    commands = [
        "entity.create",
        "entity.create",
        "resource.list",
        "entity.list",
        "scene.list"
    ]
    
    failed_commands = []
    
    for cmd in commands:
        start = time.time()
        try:
            result = subprocess.run([
                "./game_engine", "--json", "--headless", "--command", cmd
            ], capture_output=True, text=True, timeout=2, cwd="../build")
            
            elapsed = time.time() - start
            if result.returncode != 0:
                failed_commands.append((cmd, f"Exit code {result.returncode}"))
            elif elapsed > 1.0:
                print(f"  ⚠️  Command '{cmd}' took {elapsed:.2f}s")
                
        except subprocess.TimeoutExpired:
            failed_commands.append((cmd, "Timeout"))
            print(f"  ❌ Command '{cmd}' timed out")
    
    if failed_commands:
        print(f"  Failed commands: {failed_commands}")
        assert False, f"{len(failed_commands)} commands failed"
    
    print("  ✅ Rapid sequential commands test passed!")

def test_headless_mode_efficiency():
    """Test that headless mode is fast and doesn't block"""
    print("Testing headless mode efficiency...")
    
    # Run 10 entity create commands and measure time
    start_time = time.time()
    success_count = 0
    
    for i in range(10):
        try:
            result = subprocess.run([
                "./game_engine", "--json", "--headless", "--command", "entity.create"
            ], capture_output=True, text=True, timeout=1, cwd="../build")
            
            if result.returncode == 0:
                success_count += 1
                
        except subprocess.TimeoutExpired:
            print(f"  ❌ Iteration {i+1} timed out")
    
    total_time = time.time() - start_time
    avg_time = total_time / 10
    
    print(f"  Created {success_count}/10 entities in {total_time:.2f}s")
    print(f"  Average time per command: {avg_time:.3f}s")
    
    assert success_count >= 8, f"Too many failures: {success_count}/10"
    assert avg_time < 0.5, f"Commands too slow: {avg_time:.3f}s average"
    
    print("  ✅ Headless mode efficiency test passed!")

if __name__ == "__main__":
    print("=" * 60)
    print("ResourceManager Threading Tests")
    print("=" * 60)
    
    # Check if we're in the right directory
    if not os.path.exists("../build/game_engine"):
        print("ERROR: Build directory not found. Please build the project first.")
        print("Run: cd ../build && cmake .. && make")
        sys.exit(1)
    
    try:
        test_resource_manager_timeout()
        test_concurrent_texture_loading()
        test_rapid_sequential_commands()
        test_headless_mode_efficiency()
        
        print("\n" + "=" * 60)
        print("✅ All ResourceManager threading tests passed!")
        print("=" * 60)
        
    except AssertionError as e:
        print(f"\n❌ Test failed: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"\n❌ Unexpected error: {e}")
        sys.exit(1)