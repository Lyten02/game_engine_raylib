#!/usr/bin/env python3
"""Stress test config system with many rapid requests"""

import subprocess
import threading
import time
import os
import sys

def run_config_command():
    """Run a single config command"""
    subprocess.run(
        ["./game", "--json", "--headless", "--command", "config.get window.width"],
        capture_output=True, text=True, timeout=3
    )

def stress_test_config():
    """Run many config commands simultaneously"""
    
    print("Running config stress test...")
    
    # Change to build directory
    build_dir = os.path.join(os.path.dirname(__file__), "..", "build")
    os.chdir(build_dir)
    
    # Warm up - single command
    print("Warm up test...")
    result = subprocess.run(
        ["./game", "--json", "--headless", "--command", "config.get window.width"],
        capture_output=True, text=True, timeout=5
    )
    if result.returncode != 0:
        print(f"Warning: Warm up command failed: {result.stderr}")
    
    # Run 20 config commands in parallel
    threads = []
    start_time = time.time()
    
    print("Starting 20 parallel config commands...")
    for i in range(20):
        thread = threading.Thread(target=run_config_command)
        threads.append(thread)
        thread.start()
    
    # Wait for all threads to complete
    for thread in threads:
        thread.join()
    
    elapsed = time.time() - start_time
    assert elapsed < 10, f"Stress test took too long: {elapsed}s"
    print(f"✅ Stress test completed in {elapsed:.2f}s")
    
    # Test different config keys in parallel
    print("\nTesting various config keys in parallel...")
    
    def run_varied_commands(key):
        subprocess.run(
            ["./game", "--json", "--headless", "--command", f"config.get {key}"],
            capture_output=True, text=True, timeout=3
        )
    
    keys = [
        "window.width",
        "window.height", 
        "window.title",
        "engine.maxFPS",
        "invalid.key",
        "window..broken",  # Invalid key to test error handling
        "deep.nested.key.that.does.not.exist"
    ]
    
    threads = []
    start_time = time.time()
    
    for key in keys * 3:  # Run each key 3 times
        thread = threading.Thread(target=run_varied_commands, args=(key,))
        threads.append(thread)
        thread.start()
    
    for thread in threads:
        thread.join()
    
    elapsed = time.time() - start_time
    print(f"✅ Varied key test completed in {elapsed:.2f}s")
    
    # Final test: Rapid sequential commands
    print("\nRunning rapid sequential commands...")
    start_time = time.time()
    
    for i in range(50):
        result = subprocess.run(
            ["./game", "--json", "--headless", "--command", "config.get window.width"],
            capture_output=True, text=True, timeout=2
        )
    
    elapsed = time.time() - start_time
    avg_time = elapsed / 50
    print(f"✅ 50 sequential commands completed in {elapsed:.2f}s (avg: {avg_time:.3f}s per command)")
    
    assert avg_time < 0.5, f"Commands taking too long on average: {avg_time}s"

if __name__ == "__main__":
    try:
        stress_test_config()
        print("\n✅ All stress tests passed!")
    except Exception as e:
        print(f"\n❌ Stress test failed: {e}")
        sys.exit(1)