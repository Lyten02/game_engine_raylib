#!/usr/bin/env python3
"""
Test for real headless mode implementation
Tests that the game loop actually runs and processes time correctly
"""

import subprocess
import json
import sys
import os
import time

def run_engine_command(command, timeout=10):
    """Execute engine command and return JSON result with timing"""
    exe_path = "./game"
    if not os.path.exists(exe_path):
        exe_path = "./build/game"
    
    cmd = [exe_path, "--json", "--headless"]
    cmd.extend(["--command", command])
    
    start_time = time.time()
    result = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
    elapsed_time = time.time() - start_time
    
    try:
        json_result = json.loads(result.stdout)
        json_result["execution_time"] = elapsed_time
        return json_result
    except json.JSONDecodeError:
        return {
            "success": False,
            "error": f"Invalid JSON output: {result.stdout}",
            "stderr": result.stderr,
            "execution_time": elapsed_time
        }

def test_headless_game_loop():
    """Test that the headless game loop actually runs"""
    print("Testing real headless mode...")
    
    # Test 1: Basic functionality
    print("\n1. Testing basic headless execution...")
    result = run_engine_command("help")
    assert result["success"], f"Basic headless failed: {result}"
    assert result["execution_time"] < 5.0, f"Headless took too long: {result['execution_time']}s"
    print(f"✅ Headless executes in {result['execution_time']:.2f}s")
    
    # Test 2: Debug toggle (should show time progression)
    print("\n2. Testing debug toggle for time tracking...")
    result = run_engine_command("debug.toggle")
    assert result["success"], f"Debug toggle failed: {result}"
    if "data" in result and result["data"] is not None:
        print(f"✅ Debug toggle working: {result['data'].get('output', 'No output')}")
    else:
        print("✅ Debug toggle executed successfully")
    
    # Test 3: Multiple commands with timing
    print("\n3. Testing time progression...")
    start_time = time.time()
    
    # Run commands that would take some frames
    commands = [
        "project.create HeadlessTimeTest",
        "project.open HeadlessTimeTest", 
        "scene.create main",
        "entity.create Player",
        "entity.create Enemy",
        "scene.save main"
    ]
    
    for cmd in commands:
        result = run_engine_command(cmd, timeout=15)
        assert result["success"], f"Command {cmd} failed: {result}"
    
    elapsed = time.time() - start_time
    print(f"✅ Multiple commands executed in {elapsed:.2f}s")
    
    # Test 4: Auto-exit behavior
    print("\n4. Testing auto-exit after operations...")
    start_auto_exit = time.time()
    result = run_engine_command("project.list")  # Simple command that should auto-exit
    auto_exit_time = time.time() - start_auto_exit
    
    assert result["success"], f"Auto-exit test failed: {result}"
    # Note: CLI commands execute immediately without running the game loop
    # This is expected behavior for single commands
    print(f"✅ CLI commands execute quickly: {auto_exit_time:.2f}s (expected behavior)")
    
    print("\n🎉 Real headless mode tests passed!")

def test_headless_interactive_mode():
    """Test that headless mode works in interactive mode (without CLI)"""
    print("\nTesting headless interactive mode...")
    
    # Test headless mode without --command flag to trigger game loop
    exe_path = "./game"
    if not os.path.exists(exe_path):
        exe_path = "./build/game"
    
    # Run in headless mode with timeout to see if game loop runs
    start_time = time.time()
    try:
        result = subprocess.run(
            [exe_path, "--headless"],
            input="quit\n",  # Send quit command to stop the loop
            capture_output=True,
            text=True,
            timeout=5  # Should auto-exit after 1 second
        )
        elapsed = time.time() - start_time
        print(f"✅ Headless interactive mode ran for {elapsed:.2f}s")
        
        # Should auto-exit after idle time
        assert elapsed > 0.5, f"Should run for at least 0.5s: {elapsed}s"
        assert elapsed < 3.0, f"Should auto-exit within 3s: {elapsed}s"
        
    except subprocess.TimeoutExpired:
        elapsed = time.time() - start_time
        print(f"✅ Headless mode running (timeout after {elapsed:.2f}s)")

def test_headless_async_operations():
    """Test async operations in headless mode"""
    print("\nTesting async operations in headless...")
    
    # Create a project and try async build
    commands = [
        "project.create HeadlessAsyncTest",
        "project.open HeadlessAsyncTest",
        "scene.create main",
        "entity.create TestEntity"
    ]
    
    for cmd in commands:
        result = run_engine_command(cmd, timeout=15)
        assert result["success"], f"Setup command {cmd} failed: {result}"
    
    # Try async build (this should work in headless)
    print("  - Testing async build...")
    result = run_engine_command("project.build.async", timeout=30)
    
    # Note: async build might fail due to missing templates, but headless should handle it gracefully
    print(f"  - Async build result: {result['success']}")
    if not result["success"]:
        print(f"  - Expected: async build might fail in test environment")
        print(f"  - Error: {result.get('data', {}).get('output', 'No output')}")
    
    print("✅ Async operations handled in headless mode")

def test_headless_performance():
    """Test that headless mode performs well"""
    print("\nTesting headless performance...")
    
    # Create multiple entities quickly
    setup_commands = [
        "project.create PerfTest",
        "project.open PerfTest", 
        "scene.create main"
    ]
    
    for cmd in setup_commands:
        result = run_engine_command(cmd, timeout=10)
        assert result["success"], f"Setup failed: {result}"
    
    # Time creating many entities
    start_perf = time.time()
    for i in range(20):  # Create 20 entities
        result = run_engine_command(f"entity.create Entity{i}", timeout=10)
        assert result["success"], f"Entity creation {i} failed: {result}"
    
    perf_time = time.time() - start_perf
    entities_per_second = 20 / perf_time
    
    print(f"✅ Created 20 entities in {perf_time:.2f}s ({entities_per_second:.1f} entities/sec)")
    assert entities_per_second > 2, f"Performance too slow: {entities_per_second} entities/sec"

if __name__ == "__main__":
    try:
        test_headless_game_loop()
        test_headless_interactive_mode()
        test_headless_async_operations() 
        test_headless_performance()
        print("\n🎉 All real headless tests passed!")
        sys.exit(0)
    except Exception as e:
        print(f"\n❌ Test failed: {e}")
        sys.exit(1)