#!/usr/bin/env python3

import subprocess
import json
import sys
import os

def test_config_malformed_keys():
    """Test config with malformed keys that could cause infinite recursion"""
    
    test_cases = [
        "..invalid",
        "invalid..",
        "a..b",
        "a.b..c",
        "." * 50,  # Too many dots
        "a." * 20,  # Too deep nesting
        "invalid@key",  # Invalid characters
        "key#with$special!chars",  # More invalid characters
        "",  # Empty key
        "very_long_key_" + "x" * 200,  # Too long key
    ]
    
    for malformed_key in test_cases:
        print(f"Testing malformed key: '{malformed_key}'")
        
        # Test get operation - should now return error
        result = subprocess.run([
            "./game", "--json", "--headless", "--command", 
            f"config.get \"{malformed_key}\""
        ], capture_output=True, text=True, timeout=5)
        
        if result.returncode == 0:
            response = json.loads(result.stdout)
            # Now we expect either success=false OR error message in output OR usage message for empty key
            error_text = response.get("error", "") + response.get("output", "")
            assert (not response.get("success", True) or 
                   "error" in error_text.lower() or
                   "invalid" in error_text.lower() or
                   "usage:" in error_text.lower()), \
                f"Should handle malformed key with error: {malformed_key}"
        
        # Test set operation - should now return error
        result = subprocess.run([
            "./game", "--json", "--headless", "--command", 
            f"config.set \"{malformed_key}\" \"value\""
        ], capture_output=True, text=True, timeout=5)
        
        if result.returncode == 0:
            response = json.loads(result.stdout)
            error_text = response.get("error", "") + response.get("output", "")
            assert (not response.get("success", True) or 
                   "error" in error_text.lower() or
                   "invalid" in error_text.lower() or
                   "usage:" in error_text.lower()), \
                f"Should handle malformed key with error: {malformed_key}"

def test_config_deep_nesting():
    """Test config with very deep nesting"""
    
    # Create a deeply nested key (should be rejected)
    deep_key = ".".join([f"level{i}" for i in range(15)])
    
    print(f"Testing deeply nested key: {deep_key}")
    
    result = subprocess.run([
        "./game", "--json", "--headless", "--command", 
        f"config.get \"{deep_key}\""
    ], capture_output=True, text=True, timeout=5)
    
    # Should handle gracefully without hanging (exit code 0 or 1)
    assert result.returncode in [0, 1], "Command crashed with deep nesting"
    
    response = json.loads(result.stdout)
    # Should return error for overly deep keys
    error_text = response.get("error", "") + response.get("output", "")
    assert ("error" in error_text.lower() or
           "invalid" in error_text.lower()), \
        "Should reject overly deep keys"

def test_config_stress_keys():
    """Stress test with many rapid config operations"""
    
    print("Running config stress test...")
    
    for i in range(100):
        key = f"test.key.{i % 10}"  # Reuse some keys
        
        # Get operation
        result = subprocess.run([
            "./game", "--json", "--headless", "--command", 
            f"config.get \"{key}\""
        ], capture_output=True, text=True, timeout=2)
        
        # Should not hang or crash
        assert result.returncode == 0, f"Config get operation {i} failed"
        
        # Set operation (every 5th iteration)
        if i % 5 == 0:
            result = subprocess.run([
                "./game", "--json", "--headless", "--command", 
                f"config.set \"{key}\" {i}"
            ], capture_output=True, text=True, timeout=2)
            
            assert result.returncode == 0, f"Config set operation {i} failed"
    
    print("✓ Stress test completed successfully")

def test_config_special_cases():
    """Test special edge cases"""
    
    special_cases = [
        (".", "Single dot"),
        ("..", "Double dot"),
        ("...", "Triple dot"),
        ("a.", "Trailing dot"),
        (".a", "Leading dot"),
        ("a..b..c", "Multiple double dots"),
        ("_underscore_only", "Valid underscore key"),
        ("123numeric", "Numeric start"),
        ("key.123", "Numeric part"),
        ("UPPERCASE.lowercase", "Mixed case"),
    ]
    
    for key, description in special_cases:
        print(f"Testing {description}: '{key}'")
        
        result = subprocess.run([
            "./game", "--json", "--headless", "--command", 
            f"config.get \"{key}\""
        ], capture_output=True, text=True, timeout=5)
        
        assert result.returncode in [0, 1], f"Command crashed with {description}"
        
        response = json.loads(result.stdout)
        error_text = response.get("error", "") + response.get("output", "")
        
        # Check if key validation is working properly
        if key in ["_underscore_only", "123numeric", "key.123", "UPPERCASE.lowercase"]:
            # These should be valid
            assert "error" not in error_text.lower() and "invalid" not in error_text.lower(), \
                f"Valid key rejected: {key}"
        else:
            # These should be invalid
            assert "error" in error_text.lower() or "invalid" in error_text.lower(), \
                f"Invalid key accepted: {key}"

def test_config_boundary_values():
    """Test boundary value cases"""
    
    # Test maximum allowed key length (100 characters)
    max_length_key = "a" * 99  # Just under the limit
    too_long_key = "a" * 101  # Just over the limit
    
    print("Testing key length boundaries...")
    
    # Test valid maximum length
    result = subprocess.run([
        "./game", "--json", "--headless", "--command", 
        f"config.get \"{max_length_key}\""
    ], capture_output=True, text=True, timeout=5)
    
    assert result.returncode in [0, 1], "Failed with maximum length key"
    response = json.loads(result.stdout)
    output = response.get("output", "")
    assert "error" not in output.lower() and "invalid" not in output.lower(), \
        "Valid max length key rejected"
    
    # Test too long key
    result = subprocess.run([
        "./game", "--json", "--headless", "--command", 
        f"config.get \"{too_long_key}\""
    ], capture_output=True, text=True, timeout=5)
    
    assert result.returncode in [0, 1], "Crashed with too long key"
    response = json.loads(result.stdout)
    error_text = response.get("error", "") + response.get("output", "")
    assert "error" in error_text.lower() or "invalid" in error_text.lower(), \
        "Too long key should be rejected"
    
    # Test maximum nesting depth (10 levels)
    max_depth_key = ".".join([f"level{i}" for i in range(9)])  # 9 dots = 10 parts
    too_deep_key = ".".join([f"level{i}" for i in range(11)])  # 10 dots = 11 parts
    
    print("Testing nesting depth boundaries...")
    
    # Test valid maximum depth
    result = subprocess.run([
        "./game", "--json", "--headless", "--command", 
        f"config.get \"{max_depth_key}\""
    ], capture_output=True, text=True, timeout=5)
    
    assert result.returncode in [0, 1], "Failed with maximum depth key"
    response = json.loads(result.stdout)
    output = response.get("output", "")
    assert "error" not in output.lower() and "invalid" not in output.lower(), \
        "Valid max depth key rejected"
    
    # Test too deep key
    result = subprocess.run([
        "./game", "--json", "--headless", "--command", 
        f"config.get \"{too_deep_key}\""
    ], capture_output=True, text=True, timeout=5)
    
    assert result.returncode in [0, 1], "Crashed with too deep key"
    response = json.loads(result.stdout)
    error_text = response.get("error", "") + response.get("output", "")
    assert "error" in error_text.lower() or "invalid" in error_text.lower(), \
        "Too deep key should be rejected"

if __name__ == "__main__":
    # Change to build directory if not already there
    if os.path.basename(os.getcwd()) != "build":
        os.chdir("build")
    
    try:
        test_config_malformed_keys()
        print("✅ Malformed keys test passed")
        
        test_config_deep_nesting()
        print("✅ Deep nesting test passed")
        
        test_config_stress_keys()
        print("✅ Stress test passed")
        
        test_config_special_cases()
        print("✅ Special cases test passed")
        
        test_config_boundary_values()
        print("✅ Boundary values test passed")
        
        print("\n✅ All config edge case tests passed!")
    except Exception as e:
        print(f"\n❌ Config edge case test failed: {e}")
        sys.exit(1)