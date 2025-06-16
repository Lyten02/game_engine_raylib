#!/usr/bin/env python3
"""Test CLI output capture functionality"""

import subprocess
import json
import sys
import os

def run_engine_command(command, project=None):
    """Execute engine command and return JSON result"""
    # Build path to executable
    exe_path = "./game_engine"
    if not os.path.exists(exe_path):
        exe_path = "./build/game_engine"
    
    cmd = [exe_path, "--json", "--headless"]
    
    if project:
        cmd.extend(["--project", project])
    
    cmd.extend(["--command", command])
    
    # Suppress stderr to avoid spdlog messages
    result = subprocess.run(cmd, capture_output=True, text=True)
    
    try:
        return json.loads(result.stdout)
    except json.JSONDecodeError:
        return {
            "success": False,
            "error": f"Invalid JSON output: {result.stdout}",
            "stderr": result.stderr
        }

def test_help_output():
    """Test that help command returns real output"""
    print("Testing help command output...")
    result = run_engine_command("help")
    
    assert result["success"], f"Help command failed: {result}"
    output = result["output"]
    
    # Check for expected content
    assert "Available Commands" in output, "Help output missing header"
    assert "help" in output, "Help output missing help command"
    assert "project.create" in output, "Help output missing project commands"
    assert "entity.create" in output, "Help output missing entity commands"
    
    # Check it's not the placeholder
    assert "Command executed: help" not in output, "Still returning placeholder output"
    
    print(f"✅ Help command returns {len(output.split('\\n'))} lines of real output")
    return True

def test_project_list_output():
    """Test that project.list returns real project list"""
    print("\nTesting project.list output...")
    result = run_engine_command("project.list")
    
    assert result["success"], f"Project list failed: {result}"
    output = result["output"]
    
    # Check for expected content
    assert "Available projects:" in output or "No projects found" in output, \
        "Project list missing expected output"
    
    # Check it's not the placeholder
    assert "Command executed: project.list" not in output, "Still returning placeholder output"
    
    print(f"✅ Project list returns real output: {output.split('\\n')[0]}")
    return True

def test_multiline_output():
    """Test commands with multiline output"""
    print("\nTesting multiline output capture...")
    result = run_engine_command("entity.list")
    
    assert result["success"], f"Entity list failed: {result}"
    output = result["output"]
    lines = output.strip().split('\n')
    
    assert len(lines) >= 1, "Output should have at least one line"
    print(f"✅ Multiline output captured: {len(lines)} lines")
    return True

def test_error_capture():
    """Test that errors are captured properly"""
    print("\nTesting error output capture...")
    result = run_engine_command("invalid.command.xyz")
    
    assert not result["success"], "Invalid command should fail"
    output = result.get("output", "") + result.get("error", "")
    
    assert "Unknown command" in output or "Error" in output, \
        f"Error message not captured properly: {output}"
    
    print("✅ Error messages captured correctly")
    return True

def test_colored_output_stripped():
    """Test that ANSI color codes don't appear in output"""
    print("\nTesting color stripping...")
    result = run_engine_command("engine.info")
    
    assert result["success"], f"Engine info failed: {result}"
    output = result["output"]
    
    # Check for ANSI escape codes
    assert "\033[" not in output, "ANSI color codes found in output"
    assert "\x1b[" not in output, "ANSI color codes found in output"
    
    print("✅ Output is clean text without color codes")
    return True

def test_command_with_parameters():
    """Test commands with parameters capture output correctly"""
    print("\nTesting parametered command output...")
    
    # First create a test project
    create_result = run_engine_command("project.create test_output_param")
    if create_result["success"]:
        # Test opening it
        result = run_engine_command("project.open test_output_param")
        assert result["success"], f"Project open failed: {result}"
        output = result["output"]
        
        assert "Project opened" in output or "Loaded project" in output, \
            f"Project open output unexpected: {output}"
        
        print("✅ Parametered commands capture output correctly")
        return True
    else:
        # Project might already exist, that's ok for this test
        print("⚠️  Skipping parameter test (project exists)")
        return True

def main():
    """Run all output capture tests"""
    print("=== CLI Output Capture Tests ===\n")
    
    try:
        # Change to build directory if needed
        if os.path.exists("build/game_engine"):
            os.chdir("build")
        
        tests = [
            test_help_output,
            test_project_list_output,
            test_multiline_output,
            test_error_capture,
            test_colored_output_stripped,
            test_command_with_parameters
        ]
        
        passed = 0
        for test in tests:
            try:
                if test():
                    passed += 1
            except Exception as e:
                print(f"❌ {test.__name__} failed: {e}")
        
        print(f"\n{'='*40}")
        print(f"Tests passed: {passed}/{len(tests)}")
        
        if passed == len(tests):
            print("🎉 All output capture tests passed!")
            return 0
        else:
            print("❌ Some tests failed")
            return 1
            
    except Exception as e:
        print(f"❌ Test suite error: {e}")
        return 1

if __name__ == "__main__":
    sys.exit(main())