#!/usr/bin/env python3

import subprocess
import json
import sys
import os

def test_config_get_invalid_keys():
    """Test that config.get properly rejects invalid keys"""
    
    invalid_keys = ["..invalid", "invalid..", "a..b", "key@invalid"]
    
    for key in invalid_keys:
        result = subprocess.run([
            "./game", "--json", "--headless", "--command", 
            f"config.get \"{key}\""
        ], capture_output=True, text=True, timeout=5)
        
        # Command may exit with error code when reporting error
        assert result.returncode in [0, 1], f"Command crashed for key: {key}"
        
        response = json.loads(result.stdout)
        # Check either error field (when success=false) or output field
        error_text = response.get("error", "") + response.get("output", "")
        assert ("error" in error_text.lower() or 
               "invalid" in error_text.lower()), \
            f"Should report error for invalid key: {key}"

def test_config_set_invalid_keys():
    """Test that config.set properly rejects invalid keys"""
    
    invalid_keys = ["..invalid", "invalid..", "a..b", "key@invalid"]
    
    for key in invalid_keys:
        result = subprocess.run([
            "./game", "--json", "--headless", "--command", 
            f"config.set \"{key}\" \"test_value\""
        ], capture_output=True, text=True, timeout=5)
        
        # Command may exit with error code when reporting error
        assert result.returncode in [0, 1], f"Command crashed for key: {key}"
        
        response = json.loads(result.stdout)
        # Check either error field (when success=false) or output field
        error_text = response.get("error", "") + response.get("output", "")
        assert ("error" in error_text.lower() or 
               "invalid" in error_text.lower()), \
            f"Should report error for invalid key: {key}"

def test_config_valid_keys_still_work():
    """Test that valid keys continue to work after validation changes"""
    
    valid_keys = ["window.width", "console.font_size", "valid_key", "nested.valid.key"]
    
    for key in valid_keys:
        # Test get
        result = subprocess.run([
            "./game", "--json", "--headless", "--command", 
            f"config.get \"{key}\""
        ], capture_output=True, text=True, timeout=5)
        
        assert result.returncode == 0, f"Valid key should work: {key}"
        
        response = json.loads(result.stdout)
        # Check both error field and output field
        error_text = response.get("error", "") + response.get("output", "")
        assert "error" not in error_text.lower() and "invalid" not in error_text.lower(), \
            f"Valid key should not produce error: {key}"

def test_helpful_error_messages():
    """Test that error messages are helpful and informative"""
    
    result = subprocess.run([
        "./game", "--json", "--headless", "--command", 
        "config.get \"..invalid\""
    ], capture_output=True, text=True, timeout=5)
    
    response = json.loads(result.stdout)
    # Check both error field and output field
    error_text = (response.get("error", "") + response.get("output", "")).lower()
    
    # Check for helpful error information
    assert any(phrase in error_text for phrase in [
        "invalid", "format", "rules", "dots"
    ]), "Error message should be informative"

if __name__ == "__main__":
    # Change to build directory if not already there
    if os.path.basename(os.getcwd()) != "build":
        os.chdir("build")
    
    try:
        test_config_get_invalid_keys()
        test_config_set_invalid_keys()
        test_config_valid_keys_still_work()
        test_helpful_error_messages()
        print("✅ All config CLI validation tests passed")
    except Exception as e:
        print(f"❌ Config CLI validation test failed: {e}")
        sys.exit(1)