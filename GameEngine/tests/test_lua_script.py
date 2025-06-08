#!/usr/bin/env python3
"""
Test that scripts/test.lua executes properly and all Lua bindings work
"""

import sys
import os
import subprocess
import time
import tempfile

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from test_utils import run_cli_command, build_engine_if_needed, find_executable

def test_lua_script():
    """Test Lua script execution and bindings"""
    print("=" * 60)
    print("Testing Lua script execution...")
    print("=" * 60)
    
    # Build engine if needed
    build_engine_if_needed()
    
    # Find the executable
    exe = find_executable()
    if not exe:
        print("ERROR: Could not find game executable")
        return False
    
    # Test 1: Check that test.lua exists in build directory
    print("\n1. Checking test.lua in build directory...")
    build_dir = os.path.dirname(exe)
    if exe.endswith('.exe') and ('Debug' in build_dir or 'Release' in build_dir):
        build_dir = os.path.dirname(build_dir)
    
    test_lua_path = os.path.join(build_dir, 'scripts', 'test.lua')
    
    if not os.path.exists(test_lua_path):
        print(f"FAIL: test.lua not found at {test_lua_path}")
        return False
    
    print(f"PASS: Found test.lua at {test_lua_path}")
    
    # Test 2: Run a simple Lua script via CLI
    print("\n2. Testing Lua script execution via CLI...")
    
    # Create a simple test script
    with tempfile.NamedTemporaryFile(mode='w', suffix='.lua', delete=False) as f:
        f.write('''
log_info("Test script executed successfully")
return "Script completed"
''')
        temp_script = f.name
    
    try:
        result = run_cli_command(['script', 'run', temp_script])
        if result['success']:
            if "Test script executed successfully" in result['output']:
                print("PASS: Lua script executed and logging works")
            else:
                print(f"FAIL: Expected log output not found: {result['output']}")
                return False
        else:
            print(f"FAIL: Script execution failed: {result['error']}")
            return False
    finally:
        os.unlink(temp_script)
    
    # Test 3: Test Vector3 bindings
    print("\n3. Testing Vector3 Lua bindings...")
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.lua', delete=False) as f:
        f.write('''
-- Test Vector3 creation and operations
local v1 = Vector3(1, 2, 3)
local v2 = Vector3(4, 5, 6)
local v3 = v1 + v2

log_info(string.format("v1: %.1f, %.1f, %.1f", v1.x, v1.y, v1.z))
log_info(string.format("v2: %.1f, %.1f, %.1f", v2.x, v2.y, v2.z))
log_info(string.format("v3: %.1f, %.1f, %.1f", v3.x, v3.y, v3.z))

-- Test Vector3 operations
if v3.x == 5 and v3.y == 7 and v3.z == 9 then
    log_info("Vector3 addition: PASS")
else
    log_error("Vector3 addition: FAIL")
end
''')
        temp_script = f.name
    
    try:
        result = run_cli_command(['script', 'run', temp_script])
        if result['success']:
            if "Vector3 addition: PASS" in result['output']:
                print("PASS: Vector3 bindings work correctly")
            else:
                print(f"FAIL: Vector3 operations failed: {result['output']}")
                return False
        else:
            print(f"FAIL: Vector3 test failed: {result['error']}")
            return False
    finally:
        os.unlink(temp_script)
    
    # Test 4: Test Transform bindings
    print("\n4. Testing Transform Lua bindings...")
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.lua', delete=False) as f:
        f.write('''
-- Test Transform creation and access
local transform = Transform()
transform.position.x = 10
transform.position.y = 20
transform.position.z = 30
transform.scale.x = 2
transform.scale.y = 2
transform.scale.z = 2

log_info(string.format("Position: %.1f, %.1f, %.1f", 
    transform.position.x, transform.position.y, transform.position.z))
log_info(string.format("Scale: %.1f, %.1f, %.1f",
    transform.scale.x, transform.scale.y, transform.scale.z))

if transform.position.x == 10 and transform.scale.x == 2 then
    log_info("Transform test: PASS")
else
    log_error("Transform test: FAIL")
end
''')
        temp_script = f.name
    
    try:
        result = run_cli_command(['script', 'run', temp_script])
        if result['success']:
            if "Transform test: PASS" in result['output']:
                print("PASS: Transform bindings work correctly")
            else:
                print(f"FAIL: Transform test failed: {result['output']}")
                return False
        else:
            print(f"FAIL: Transform test failed: {result['error']}")
            return False
    finally:
        os.unlink(temp_script)
    
    # Test 5: Test all logging functions
    print("\n5. Testing Lua logging functions...")
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.lua', delete=False) as f:
        f.write('''
log_info("Info message test")
log_warn("Warning message test")
log_error("Error message test")
log_debug("Debug message test")
log_info("All logging functions tested")
''')
        temp_script = f.name
    
    try:
        result = run_cli_command(['script', 'run', temp_script])
        if result['success']:
            output = result['output'] + result['error']  # Check both streams
            
            tests_passed = True
            if "Info message test" not in output:
                print("FAIL: log_info not working")
                tests_passed = False
            if "Warning message test" not in output:
                print("FAIL: log_warn not working")
                tests_passed = False
            if "Error message test" not in output:
                print("FAIL: log_error not working")
                tests_passed = False
            # Debug might not show depending on log level
            
            if tests_passed:
                print("PASS: All logging functions work correctly")
            else:
                print(f"Output: {output}")
                return False
        else:
            print(f"FAIL: Logging test failed: {result['error']}")
            return False
    finally:
        os.unlink(temp_script)
    
    # Test 6: Run the actual test.lua script
    print("\n6. Running scripts/test.lua...")
    
    result = run_cli_command(['script', 'run', 'scripts/test.lua'])
    if result['success']:
        output = result['output']
        
        # Check for expected outputs from test.lua
        expected_outputs = [
            "Hello from Lua script!",
            "Test function called from Lua",
            "Created position:",
            "Created velocity:",
            "Position + Velocity =",
            "Transform position:",
            "Transform scale:",
            "This is a warning from Lua",
            "Lua script initialization complete!"
        ]
        
        all_found = True
        for expected in expected_outputs:
            if expected not in output:
                print(f"FAIL: Expected output not found: '{expected}'")
                all_found = False
        
        if all_found:
            print("PASS: test.lua executed successfully with all expected outputs")
        else:
            print(f"Output received:\n{output}")
            return False
    else:
        print(f"FAIL: test.lua execution failed: {result['error']}")
        return False
    
    # Test 7: Test error handling
    print("\n7. Testing Lua error handling...")
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.lua', delete=False) as f:
        f.write('''
-- This should cause an error
local result = nil + 5
''')
        temp_script = f.name
    
    try:
        result = run_cli_command(['script', 'run', temp_script])
        if not result['success']:
            if "error" in result['error'].lower() or "error" in result['output'].lower():
                print("PASS: Lua errors are properly reported")
            else:
                print("WARNING: Script failed but no clear error message")
        else:
            print("FAIL: Script with error should have failed")
            return False
    finally:
        os.unlink(temp_script)
    
    print("\n" + "=" * 60)
    print("All Lua script tests passed!")
    print("=" * 60)
    return True

if __name__ == "__main__":
    # Ensure test_utils.py exists
    test_utils_path = os.path.join(os.path.dirname(__file__), 'test_utils.py')
    if not os.path.exists(test_utils_path):
        print("ERROR: test_utils.py not found. Run test_config_loading.py first to create it.")
        sys.exit(1)
    
    success = test_lua_script()
    sys.exit(0 if success else 1)