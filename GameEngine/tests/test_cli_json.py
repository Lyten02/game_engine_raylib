#!/usr/bin/env python3
"""Test CLI JSON functionality for GameEngine"""

import subprocess
import json
import sys
import os

# Path to the game executable
GAME_EXE = "../build/game"

def run_cli_command(args):
    """Run a CLI command and return the result"""
    cmd = [GAME_EXE] + args
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=5)
        return {
            "success": result.returncode == 0,
            "stdout": result.stdout,
            "stderr": result.stderr,
            "returncode": result.returncode
        }
    except subprocess.TimeoutExpired:
        return {
            "success": False,
            "stdout": "",
            "stderr": "Command timed out",
            "returncode": -1
        }
    except Exception as e:
        return {
            "success": False,
            "stdout": "",
            "stderr": str(e),
            "returncode": -1
        }

def test_help():
    """Test --help flag"""
    print("Testing --help...")
    result = run_cli_command(["--help"])
    if result["success"]:
        print("✅ Help command works")
        print(f"Output length: {len(result['stdout'])} characters")
        # Check if help contains expected content
        if "Usage:" in result["stdout"] and "Options:" in result["stdout"]:
            print("✅ Help contains expected sections")
        else:
            print("❌ Help output missing expected sections")
    else:
        print("❌ Help command failed")
        print(f"Error: {result['stderr']}")
    print()

def test_version():
    """Test --version flag"""
    print("Testing --version...")
    result = run_cli_command(["--version"])
    if result["success"]:
        print("✅ Version command works")
        print(f"Version output: {result['stdout'].strip()}")
    else:
        print("❌ Version command failed")
        print(f"Error: {result['stderr']}")
    print()

def test_json_help():
    """Test JSON output with help command"""
    print("Testing JSON output with help command...")
    result = run_cli_command(["--json", "--command", "help"])
    if result["success"]:
        try:
            json_data = json.loads(result["stdout"])
            print("✅ JSON output is valid")
            print(f"JSON keys: {list(json_data.keys())}")
            
            # Check expected fields
            if "success" in json_data:
                print(f"  Success: {json_data['success']}")
            if "output" in json_data:
                print(f"  Output length: {len(json_data.get('output', ''))}")
            if "data" in json_data:
                print(f"  Data present: Yes")
                
        except json.JSONDecodeError as e:
            print("❌ JSON output is invalid")
            print(f"JSON Error: {e}")
            print(f"Raw output: {result['stdout'][:200]}...")
    else:
        print("❌ JSON help command failed")
        print(f"Error: {result['stderr']}")
    print()

def test_json_project_list():
    """Test JSON output with project.list command"""
    print("Testing JSON output with project.list...")
    result = run_cli_command(["--json", "--headless", "--command", "project.list"])
    if result["success"]:
        try:
            json_data = json.loads(result["stdout"])
            print("✅ project.list JSON output is valid")
            print(f"JSON structure: {json.dumps(json_data, indent=2)[:300]}...")
        except json.JSONDecodeError:
            print("❌ project.list JSON output is invalid")
            print(f"Raw output: {result['stdout'][:200]}...")
    else:
        print("❌ project.list command failed")
        print(f"Error: {result['stderr']}")
    print()

def test_headless_project_create():
    """Test creating a project in headless mode"""
    print("Testing headless project creation...")
    project_name = "test_cli_auto_project"
    
    result = run_cli_command(["--json", "--headless", "--command", f"project.create {project_name}"])
    if result["success"]:
        try:
            json_data = json.loads(result["stdout"])
            print(f"✅ Project creation returned valid JSON")
            print(f"Success: {json_data.get('success', False)}")
            print(f"Message: {json_data.get('output', 'No message')}")
            
            # Check if project was actually created
            projects_dir = "../projects"
            project_path = os.path.join(projects_dir, project_name)
            if os.path.exists(project_path):
                print(f"✅ Project directory created at: {project_path}")
                # Clean up
                import shutil
                shutil.rmtree(project_path)
                print("✅ Test project cleaned up")
            else:
                print("❌ Project directory was not created")
                
        except json.JSONDecodeError:
            print("❌ Project creation JSON output is invalid")
            print(f"Raw output: {result['stdout']}")
    else:
        print("❌ Project creation failed")
        print(f"Error: {result['stderr']}")
    print()

def test_batch_commands():
    """Test batch command execution"""
    print("Testing batch commands...")
    commands = ["help", "project.list"]
    
    # Create batch command string
    batch_args = ["--json", "--headless", "--batch"] + commands
    
    result = run_cli_command(batch_args)
    if result["success"]:
        try:
            json_data = json.loads(result["stdout"])
            print("✅ Batch execution returned valid JSON")
            
            if "data" in json_data and "results" in json_data.get("data", {}):
                results = json_data["data"]["results"]
                print(f"✅ Executed {len(results)} commands")
                for i, cmd_result in enumerate(results):
                    print(f"  Command {i+1}: {cmd_result.get('command')} - Success: {cmd_result.get('success')}")
            else:
                print("❌ Batch results not in expected format")
                
        except json.JSONDecodeError:
            print("❌ Batch JSON output is invalid")
            print(f"Raw output: {result['stdout'][:200]}...")
    else:
        print("❌ Batch execution failed")
        print(f"Error: {result['stderr']}")
    print()

def test_invalid_command():
    """Test handling of invalid commands"""
    print("Testing invalid command handling...")
    result = run_cli_command(["--json", "--command", "invalid.command.test"])
    if result["returncode"] != 0:  # Should fail
        try:
            json_data = json.loads(result["stdout"])
            print("✅ Invalid command properly handled with JSON error")
            print(f"Success: {json_data.get('success', 'N/A')}")
            print(f"Error: {json_data.get('error', 'No error message')}")
        except json.JSONDecodeError:
            print("❌ Invalid command didn't return valid JSON")
            print(f"Raw output: {result['stdout']}")
    else:
        print("❌ Invalid command unexpectedly succeeded")
    print()

def main():
    """Run all tests"""
    print("=" * 60)
    print("GameEngine CLI JSON Test Suite")
    print("=" * 60)
    print()
    
    # Check if executable exists
    if not os.path.exists(GAME_EXE):
        print(f"❌ Error: Game executable not found at {GAME_EXE}")
        print("Please build the project first with:")
        print("  cd ../build && cmake .. && make")
        return 1
    
    # Run tests
    test_help()
    test_version()
    test_json_help()
    test_json_project_list()
    test_headless_project_create()
    test_batch_commands()
    test_invalid_command()
    
    print("=" * 60)
    print("Test suite completed!")
    print("=" * 60)
    
    return 0

if __name__ == "__main__":
    sys.exit(main())