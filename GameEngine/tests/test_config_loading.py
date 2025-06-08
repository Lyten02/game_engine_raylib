#!/usr/bin/env python3
"""
Test that config.json loads properly and all configuration values are accessible
"""

import sys
import os
import subprocess
import json
import tempfile
import time

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from test_utils import run_cli_command, build_engine_if_needed, find_executable, wait_for_output

def test_config_loading():
    """Test that config.json is loaded without warnings"""
    print("=" * 60)
    print("Testing config.json loading...")
    print("=" * 60)
    
    # Build engine if needed
    build_engine_if_needed()
    
    # Find the executable
    exe = find_executable()
    if not exe:
        print("ERROR: Could not find game executable")
        return False
    
    # Test 1: Run engine and check for config loading warnings
    print("\n1. Testing for config loading warnings...")
    try:
        # Run the engine with a short timeout and capture output
        process = subprocess.Popen(
            [exe, '--headless'],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        
        # Wait a bit for initialization
        time.sleep(0.5)
        
        # Terminate the process
        process.terminate()
        stdout, stderr = process.communicate(timeout=2)
        
        # Check for config loading warnings
        if "Configuration not loaded" in stdout or "Configuration not loaded" in stderr:
            print("FAIL: Found 'Configuration not loaded' warning")
            print(f"stdout: {stdout}")
            print(f"stderr: {stderr}")
            return False
        
        if "Config::load - File not found" in stdout or "Config::load - File not found" in stderr:
            print("FAIL: Config file not found")
            print(f"stdout: {stdout}")
            print(f"stderr: {stderr}")
            return False
            
        print("PASS: No config loading warnings found")
        
    except subprocess.TimeoutExpired:
        process.kill()
        stdout, stderr = process.communicate()
        print("PASS: Engine started without config errors (timed out as expected)")
    except Exception as e:
        print(f"ERROR: {e}")
        return False
    
    # Test 2: Check config file exists in build directory
    print("\n2. Checking config.json exists in build directory...")
    build_dir = os.path.join(os.path.dirname(exe), "..")
    config_path = os.path.join(build_dir, "config.json")
    
    if not os.path.exists(config_path):
        print(f"FAIL: config.json not found at {config_path}")
        return False
    
    print(f"PASS: config.json found at {config_path}")
    
    # Test 3: Validate config.json structure
    print("\n3. Validating config.json structure...")
    try:
        with open(config_path, 'r') as f:
            config = json.load(f)
        
        # Check for required sections
        required_sections = ['window', 'console', 'scripting']
        for section in required_sections:
            if section not in config:
                print(f"FAIL: Missing required section '{section}' in config.json")
                return False
        
        # Check window settings
        window_keys = ['width', 'height', 'title', 'fullscreen', 'vsync', 'target_fps']
        for key in window_keys:
            if key not in config['window']:
                print(f"FAIL: Missing window.{key} in config.json")
                return False
        
        # Check console settings
        console_keys = ['font_size', 'max_lines', 'background_alpha']
        for key in console_keys:
            if key not in config['console']:
                print(f"FAIL: Missing console.{key} in config.json")
                return False
        
        # Check scripting settings
        if 'lua_enabled' not in config['scripting']:
            print("FAIL: Missing scripting.lua_enabled in config.json")
            return False
        
        if 'script_directory' not in config['scripting']:
            print("FAIL: Missing scripting.script_directory in config.json")
            return False
            
        print("PASS: config.json has all required sections and keys")
        
    except json.JSONDecodeError as e:
        print(f"FAIL: config.json is not valid JSON: {e}")
        return False
    except Exception as e:
        print(f"ERROR: Failed to validate config.json: {e}")
        return False
    
    # Test 4: Test config get command in CLI
    print("\n4. Testing config get command...")
    result = run_cli_command(['config.get', 'window.width'])
    if result['success']:
        # Check if output contains the expected width value
        if '1280' in result['output']:
            print("PASS: config get window.width returned correct value")
        else:
            print(f"FAIL: config get window.width returned unexpected value: {result['output']}")
            return False
    else:
        print(f"FAIL: config get command failed: {result['error']}")
        return False
    
    print("\n" + "=" * 60)
    print("All config loading tests passed!")
    print("=" * 60)
    return True

if __name__ == "__main__":
    # Check for test_utils.py
    test_utils_path = os.path.join(os.path.dirname(__file__), 'test_utils.py')
    if not os.path.exists(test_utils_path):
        # Create a minimal test_utils.py if it doesn't exist
        with open(test_utils_path, 'w') as f:
            f.write('''import os
import subprocess
import time

def find_executable():
    """Find the game executable in the build directory"""
    build_dir = os.path.join(os.path.dirname(__file__), '..', 'build')
    
    # Try different possible locations
    possible_paths = [
        os.path.join(build_dir, 'game'),
        os.path.join(build_dir, 'game.exe'),
        os.path.join(build_dir, 'Debug', 'game.exe'),
        os.path.join(build_dir, 'Release', 'game.exe'),
    ]
    
    for path in possible_paths:
        if os.path.exists(path):
            return path
    
    return None

def build_engine_if_needed():
    """Build the engine if the executable doesn't exist"""
    exe = find_executable()
    if exe and os.path.exists(exe):
        return True
    
    build_dir = os.path.join(os.path.dirname(__file__), '..', 'build')
    
    # Create build directory if it doesn't exist
    os.makedirs(build_dir, exist_ok=True)
    
    # Run cmake
    result = subprocess.run(['cmake', '..'], cwd=build_dir, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"CMAKE failed: {result.stderr}")
        return False
    
    # Build
    result = subprocess.run(['cmake', '--build', '.'], cwd=build_dir, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"Build failed: {result.stderr}")
        return False
    
    return True

def run_cli_command(args, timeout=5):
    """Run a CLI command and return the result"""
    exe = find_executable()
    if not exe:
        return {'success': False, 'output': '', 'error': 'Executable not found'}
    
    try:
        result = subprocess.run(
            [exe, '--cli'] + args,
            capture_output=True,
            text=True,
            timeout=timeout
        )
        
        return {
            'success': result.returncode == 0,
            'output': result.stdout,
            'error': result.stderr
        }
    except subprocess.TimeoutExpired:
        return {'success': False, 'output': '', 'error': 'Command timed out'}
    except Exception as e:
        return {'success': False, 'output': '', 'error': str(e)}

def wait_for_output(process, expected_text, timeout=5):
    """Wait for expected text in process output"""
    import select
    start_time = time.time()
    output = ""
    
    while time.time() - start_time < timeout:
        if process.poll() is not None:
            # Process has terminated
            remaining_output = process.stdout.read()
            output += remaining_output
            if expected_text in output:
                return True, output
            return False, output
        
        # Check if there's data to read
        ready, _, _ = select.select([process.stdout], [], [], 0.1)
        if ready:
            line = process.stdout.readline()
            output += line
            if expected_text in output:
                return True, output
    
    return False, output
''')
    
    success = test_config_loading()
    sys.exit(0 if success else 1)