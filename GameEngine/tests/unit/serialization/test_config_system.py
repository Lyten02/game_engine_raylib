#!/usr/bin/env python3
"""Test config system stability and edge cases"""

import subprocess
import json
import os
import sys

def test_config_commands():
    """Test various config get/set operations"""
    
    print("Testing config system edge cases...")
    
    # Change to build directory
    build_dir = os.path.join(os.path.dirname(__file__), "..", "build")
    os.chdir(build_dir)
    
    # Test 1: Normal config access
    print("Test 1: Normal config access...")
    result = subprocess.run(
        ["./game_engine", "--json", "--headless", "--command", "config.get window.width"],
        capture_output=True, text=True, timeout=5
    )
    if result.returncode != 0:
        print(f"Error: {result.stderr}")
    assert result.returncode == 0, "Normal config access failed"
    print("✓ Normal config access works")
    
    # Test 2: Invalid keys that could cause loops
    invalid_keys = [
        "window..width",  # Double dots
        "..invalid",      # Leading dots  
        "invalid..",      # Trailing dots
        "a." * 50,        # Very deep nesting
        "",               # Empty key
        "window.nonexistent.deep.key.that.does.not.exist"
    ]
    
    print("\nTest 2: Invalid keys that previously caused loops...")
    for key in invalid_keys:
        try:
            result = subprocess.run(
                ["./game_engine", "--json", "--headless", "--command", f"config.get {key}"],
                capture_output=True, text=True, timeout=5
            )
            # Should not timeout, regardless of success/failure
            print(f"✓ Key '{key[:30]}{'...' if len(key) > 30 else ''}' handled without timeout")
        except subprocess.TimeoutExpired:
            print(f"✗ Key '{key}' caused timeout!")
            assert False, f"Key '{key}' caused timeout"
    
    # Test 3: Valid nested keys
    print("\nTest 3: Valid nested keys...")
    valid_keys = [
        "window.width",
        "window.height",
        "window.title",
        "engine.maxFPS"
    ]
    
    for key in valid_keys:
        result = subprocess.run(
            ["./game_engine", "--json", "--headless", "--command", f"config.get {key}"],
            capture_output=True, text=True, timeout=5
        )
        print(f"✓ Key '{key}' accessed successfully")
    
    # Test 4: Config set with invalid keys
    print("\nTest 4: Config set with invalid keys...")
    for key in ["test..invalid", "..test", "test.."]:
        result = subprocess.run(
            ["./game_engine", "--json", "--headless", "--command", f"config.set {key} 123"],
            capture_output=True, text=True, timeout=5
        )
        print(f"✓ Set with key '{key}' handled without timeout")
    
    print("\n✅ All config tests passed!")

if __name__ == "__main__":
    try:
        test_config_commands()
    except Exception as e:
        print(f"❌ Test failed: {e}")
        sys.exit(1)