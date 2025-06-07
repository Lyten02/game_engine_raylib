#!/usr/bin/env python3
"""Run a script test file - FIXED VERSION"""
import subprocess
import sys
import json
import os

def run_script_test(script_file):
    """Run commands from a script file"""
    
    # Change to build directory
    build_dir = os.path.join(os.path.dirname(__file__), '../build')
    if not os.path.exists(os.path.join(build_dir, 'game')):
        print(f"❌ Game executable not found in {build_dir}")
        return False
    
    os.chdir(build_dir)
    
    script_path = os.path.join('..', 'tests', script_file)
    if not os.path.exists(script_path):
        print(f"❌ Script file not found: {script_path}")
        return False
    
    try:
        result = subprocess.run([
            "./game", "--json", "--headless", "--script", script_path
        ], capture_output=True, text=True, timeout=30)
        
        if result.returncode == 0:
            try:
                response = json.loads(result.stdout)
                if response.get("success"):
                    print(f"✅ Script {script_file} executed successfully")
                    return True
                else:
                    print(f"❌ Script {script_file} failed: {response.get('error', 'Unknown error')}")
                    return False
            except json.JSONDecodeError:
                print(f"❌ Invalid JSON output from script {script_file}")
                return False
        else:
            print(f"❌ Script {script_file} execution failed:")
            print(f"  Return code: {result.returncode}")
            print(f"  Stdout: {result.stdout[:500] if result.stdout else 'No stdout'}")
            print(f"  Stderr: {result.stderr[:500] if result.stderr else 'No stderr'}")
            return False
            
    except subprocess.TimeoutExpired:
        print(f"❌ Script {script_file} timed out")
        return False
    except Exception as e:
        print(f"❌ Error running script {script_file}: {e}")
        return False

def test_basic_script():
    """Test basic script functionality when no arguments provided"""
    import tempfile
    import time
    
    # Create a simple test script with unique project name
    unique_name = f"test_script_{int(time.time())}"
    script_content = f"""# Test script
project.create {unique_name}
project.open {unique_name}
project.list
project.close
"""
    
    # Write to temporary file in tests directory
    tests_dir = os.path.dirname(os.path.abspath(__file__))
    temp_script_path = os.path.join(tests_dir, f"temp_test_{unique_name}.txt")
    
    with open(temp_script_path, 'w') as f:
        f.write(script_content)
    
    try:
        # Use basename since run_script_test expects filename relative to tests/
        result = run_script_test(os.path.basename(temp_script_path))
        return result
    finally:
        if os.path.exists(temp_script_path):
            os.unlink(temp_script_path)

def main():
    if len(sys.argv) > 1:
        # If argument provided, use it
        script_file = sys.argv[1]
        success = run_script_test(script_file)
    else:
        # FIX: If no arguments, run basic test
        print("No script file provided, running basic script test...")
        success = test_basic_script()
    
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()