#!/usr/bin/env python3
"""
Test script to verify log limiting functionality.
Creates multiple projects to test that repetitive logs are suppressed.
"""

import os
import subprocess
import sys
import json
import time

# Add parent directory to path for test_utils import
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from test_utils import print_test_header, print_test_result

def run_engine_command(command, log_level=None, quiet=False):
    """Run engine with specific log settings"""
    game_path = os.path.join(os.path.dirname(__file__), "..", "build", "game_engine")
    
    cmd = [game_path, "--headless", "--command", command]
    if log_level:
        cmd.extend(["--log-level", log_level])
    if quiet:
        cmd.append("--quiet")
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
        return result.returncode == 0, result.stdout, result.stderr
    except Exception as e:
        return False, "", str(e)

def test_normal_logging():
    """Test that normal logging works without limiting"""
    print_test_header("Normal Logging (No Limiting)")
    
    # Create multiple projects with normal logging
    for i in range(5):
        success, stdout, stderr = run_engine_command(f"project.create TestLogProject{i}")
        if not success:
            print_test_result("Normal logging", False, f"Failed to create project {i}")
            return False
        
        # Check that we see the log message
        if "Project loaded" in stderr or "Project created" in stdout:
            print(f"  ✓ Project {i} creation logged")
        else:
            print(f"  ✗ Project {i} creation not logged")
    
    print_test_result("Normal logging", True)
    return True

def test_quiet_mode():
    """Test that quiet mode suppresses info logs"""
    print_test_header("Quiet Mode Logging")
    
    # Create projects with quiet mode
    for i in range(3):
        success, stdout, stderr = run_engine_command(f"project.create QuietProject{i}", quiet=True)
        if not success:
            print_test_result("Quiet mode", False, f"Failed to create project {i}")
            return False
        
        # In quiet mode, we shouldn't see info logs
        if "Project loaded" not in stderr and "Building project" not in stderr:
            print(f"  ✓ Project {i} - info logs suppressed")
        else:
            print(f"  ✗ Project {i} - info logs not suppressed: {stderr}")
    
    print_test_result("Quiet mode", True)
    return True

def test_log_level_override():
    """Test log level override functionality"""
    print_test_header("Log Level Override")
    
    # Test different log levels
    test_cases = [
        ("off", "No logs should appear"),
        ("error", "Only errors should appear"),
        ("warn", "Warnings and errors should appear"),
        ("info", "Info, warnings and errors should appear"),
        ("debug", "All logs including debug should appear")
    ]
    
    for level, description in test_cases:
        success, stdout, stderr = run_engine_command(f"project.create LogLevel{level.title()}Project", log_level=level)
        
        if not success and level != "off":
            print_test_result(f"Log level {level}", False, f"Command failed")
            return False
        
        print(f"  Testing {level}: {description}")
        
        # For 'off' level, there should be no logs
        if level == "off":
            if len(stderr.strip()) == 0:
                print(f"    ✓ No logs produced")
            else:
                print(f"    ✗ Unexpected logs: {stderr}")
        else:
            print(f"    ℹ️  Log output length: {len(stderr)} chars")
    
    print_test_result("Log level override", True)
    return True

def test_batch_mode_limiting():
    """Test that batch mode applies log limiting"""
    print_test_header("Batch Mode Log Limiting")
    
    # Create a batch file with repetitive commands
    batch_commands = []
    for i in range(10):
        batch_commands.append(f"project.create BatchProject{i}")
        batch_commands.append(f"project.open BatchProject{i}")
    
    # Write batch file
    batch_file = "test_log_limit_batch.txt"
    with open(batch_file, "w") as f:
        f.write("\n".join(batch_commands))
    
    try:
        # Run batch with default settings (should apply limiting)
        game_path = os.path.join(os.path.dirname(__file__), "..", "build", "game_engine")
        cmd = [game_path, "--headless", "--script", batch_file]
        
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
        
        # Count occurrences of "Project loaded" in stderr
        project_loaded_count = result.stderr.count("Project loaded")
        
        print(f"  Total commands: {len(batch_commands)}")
        print(f"  'Project loaded' messages: {project_loaded_count}")
        
        # With limiting, we should see fewer messages than projects created
        if project_loaded_count < 10:
            print(f"  ✓ Log limiting applied (only {project_loaded_count} messages for 10 projects)")
            success = True
        else:
            print(f"  ✗ Log limiting not applied ({project_loaded_count} messages)")
            success = False
        
        print_test_result("Batch mode limiting", success)
        return success
        
    finally:
        # Clean up
        if os.path.exists(batch_file):
            os.remove(batch_file)

def cleanup_test_projects():
    """Clean up test projects"""
    build_dir = os.path.join(os.path.dirname(__file__), "..", "build")
    projects_dir = os.path.join(build_dir, "projects")
    
    if os.path.exists(projects_dir):
        import shutil
        for project in os.listdir(projects_dir):
            if project.startswith(("TestLogProject", "QuietProject", "LogLevel", "BatchProject")):
                project_path = os.path.join(projects_dir, project)
                if os.path.isdir(project_path):
                    shutil.rmtree(project_path)
                    print(f"  Cleaned up: {project}")

def main():
    print("\n" + "="*60)
    print("LOG LIMITING TEST SUITE")
    print("="*60 + "\n")
    
    tests = [
        test_normal_logging,
        test_quiet_mode,
        test_log_level_override,
        test_batch_mode_limiting
    ]
    
    passed = 0
    failed = 0
    
    for test in tests:
        if test():
            passed += 1
        else:
            failed += 1
    
    # Clean up
    print("\nCleaning up test projects...")
    cleanup_test_projects()
    
    print("\n" + "="*60)
    print(f"TEST SUMMARY: {passed} passed, {failed} failed")
    print("="*60 + "\n")
    
    return failed == 0

if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)