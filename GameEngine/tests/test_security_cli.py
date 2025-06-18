
# Import TDD dependency resolver
try:
    from test_dependency_path_fix import get_compilation_flags, validate_test_environment
except ImportError:
    def get_compilation_flags():
        return {'includes': '', 'libs': '-lraylib -lspdlog', 'deps_dir': None}
    def validate_test_environment():
        return False, "Dependency resolver not available"

#!/usr/bin/env python3
"""Test security fixes for command injection vulnerability through CLI"""

import subprocess
import os
import sys
import json
import tempfile
import shutil

def run_cli_command(command, expected_to_fail=True):
    """Run a CLI command and return the result"""
    try:
        # Run the command
        result = subprocess.run(
            ["./game_engine", "--json", "-c", command],
            capture_output=True,
            text=True,
            timeout=5
        )
        
        # Try to parse JSON output
        try:
            output = json.loads(result.stdout)
            return {
                "success": output.get("success", False),
                "error": output.get("error", ""),
                "exit_code": result.returncode,
                "raw_output": result.stdout,
                "stderr": result.stderr
            }
        except json.JSONDecodeError:
            return {
                "success": False,
                "error": "Invalid JSON output",
                "exit_code": result.returncode,
                "raw_output": result.stdout,
                "stderr": result.stderr
            }
    except subprocess.TimeoutExpired:
        return {
            "success": False,
            "error": "Command timed out",
            "exit_code": -1,
            "raw_output": "",
            "stderr": "Timeout"
        }
    except Exception as e:
        return {
            "success": False,
            "error": str(e),
            "exit_code": -1,
            "raw_output": "",
            "stderr": str(e)
        }

def test_command_injection_prevention():
    """Test that command injection attempts are properly blocked"""
    print("=== Testing Command Injection Prevention ===\n")
    
    # Create a canary file that should NOT be deleted
    canary_file = tempfile.mktemp(prefix="security_test_canary_")
    with open(canary_file, 'w') as f:
        f.write("This file should not be deleted by command injection")
    
    injection_tests = [
        # Basic command injection attempts
        ('project.create "test; rm {}"'.format(canary_file), "Semicolon injection"),
        ('project.create "test && rm {}"'.format(canary_file), "AND injection"),
        ('project.create "test || rm {}"'.format(canary_file), "OR injection"),
        ('project.create "test | cat /etc/passwd"', "Pipe injection"),
        ('project.create "test`whoami`"', "Backtick injection"),
        ('project.create "test$(id)"', "Command substitution"),
        ('project.create "test > /tmp/pwned"', "Output redirection"),
        ('project.create "test < /etc/passwd"', "Input redirection"),
        
        # More sophisticated attempts
        ('project.create "test\\nrm {}"'.format(canary_file), "Newline injection"),
        ('project.create "test\'; DROP TABLE projects; --"', "SQL injection style"),
        ('project.create "../../../tmp/evil"', "Path traversal in name"),
        ('project.create "test\\x00hack"', "Null byte injection"),
        
        # Unicode and encoding tricks
        ('project.create "test\u0000hack"', "Unicode null"),
        ('project.create "test%00hack"', "URL encoded null"),
        ('project.create "test%3Brm%20{}"'.format(canary_file), "URL encoded command"),
    ]
    
    all_safe = True
    
    for command, description in injection_tests:
        print(f"Testing {description}...")
        result = run_cli_command(command)
        
        # Check if the command was rejected
        if result["success"]:
            print(f"  ❌ FAIL: Command succeeded when it should have been rejected!")
            print(f"     Command: {command}")
            all_safe = False
        else:
            print(f"  ✅ PASS: Command properly rejected")
            print(f"     Error: {result['error']}")
        
        # Verify our canary file still exists
        if not os.path.exists(canary_file):
            print(f"  ❌ CRITICAL: Canary file was deleted! Command injection succeeded!")
            all_safe = False
            break
    
    # Clean up
    if os.path.exists(canary_file):
        os.remove(canary_file)
        print("\n✅ Canary file survived all injection attempts")
    
    return all_safe

def test_build_command_injection():
    """Test command injection in build commands"""
    print("\n=== Testing Build Command Injection Prevention ===\n")
    
    # First create a legitimate project
    print("Creating test project...")
    result = run_cli_command("project.create SecurityBuildTest")
    if not result["success"]:
        # Clean up if it exists
        run_cli_command("project.remove SecurityBuildTest")
        result = run_cli_command("project.create SecurityBuildTest")
    
    if not result["success"]:
        print("❌ Failed to create test project")
        return False
    
    # Try to inject commands through build
    canary_file = tempfile.mktemp(prefix="build_test_canary_")
    with open(canary_file, 'w') as f:
        f.write("Build injection canary")
    
    injection_attempts = [
        'project.build "SecurityBuildTest; rm {}"'.format(canary_file),
        'project.build-fast "SecurityBuildTest && echo pwned > /tmp/pwned"',
    ]
    
    all_safe = True
    
    for command in injection_attempts:
        print(f"Testing: {command}")
        result = run_cli_command(command, expected_to_fail=False)
        
        # Build might fail for other reasons, but canary should survive
        if not os.path.exists(canary_file):
            print(f"  ❌ CRITICAL: Build command injection succeeded!")
            all_safe = False
            break
        else:
            print(f"  ✅ PASS: Canary file protected from build injection")
    
    # Clean up
    if os.path.exists(canary_file):
        os.remove(canary_file)
    
    run_cli_command("project.remove SecurityBuildTest")
    
    return all_safe

def test_path_traversal_prevention():
    """Test that path traversal attempts are blocked"""
    print("\n=== Testing Path Traversal Prevention ===\n")
    
    traversal_tests = [
        ("project.create ../EvilProject", "Parent directory traversal"),
        ("project.create ../../EvilProject", "Multiple parent traversal"),
        ("project.create /tmp/EvilProject", "Absolute path"),
        ("project.create ./../../EvilProject", "Hidden traversal"),
        ("project.create projects/../../../tmp/evil", "Mid-path traversal"),
    ]
    
    all_safe = True
    
    for command, description in traversal_tests:
        print(f"Testing {description}...")
        result = run_cli_command(command)
        
        if result["success"]:
            print(f"  ❌ FAIL: Path traversal attempt succeeded!")
            all_safe = False
            
            # Try to clean up
            project_name = command.split()[-1].strip('"')
            run_cli_command(f"project.remove {project_name}")
        else:
            print(f"  ✅ PASS: Path traversal properly blocked")
    
    return all_safe

def test_valid_operations():
    """Test that legitimate operations still work"""
    print("\n=== Testing Valid Operations ===\n")
    
    valid_tests = [
        ("project.create ValidProject123", "Alphanumeric project"),
        ("project.create Valid_Project", "Project with underscore"),
        ("project.create Valid-Project", "Project with hyphen"),
        ("project.create MyAwesome_Game-2024", "Complex valid name"),
    ]
    
    all_passed = True
    created_projects = []
    
    for command, description in valid_tests:
        print(f"Testing {description}...")
        result = run_cli_command(command, expected_to_fail=False)
        
        if result["success"]:
            print(f"  ✅ PASS: Valid project created successfully")
            project_name = command.split()[-1].strip('"')
            created_projects.append(project_name)
        else:
            print(f"  ❌ FAIL: Valid operation was blocked!")
            print(f"     Error: {result['error']}")
            all_passed = False
    
    # Clean up created projects
    for project in created_projects:
        run_cli_command(f"project.remove {project}")
    
    return all_passed

def main():
    print("=" * 60)
    print("Command Injection Security Tests - CLI")
    print("=" * 60)
    print()
    
    # Check if game executable exists
    if not os.path.exists("./game_engine"):
        print("❌ ERROR: game executable not found in current directory")
        print("Please run this test from the build directory")
        return 1
    
    # Run all tests
    tests = [
        ("Command Injection Prevention", test_command_injection_prevention),
        ("Build Command Injection", test_build_command_injection),
        ("Path Traversal Prevention", test_path_traversal_prevention),
        ("Valid Operations", test_valid_operations),
    ]
    
    results = []
    
    for test_name, test_func in tests:
        try:
            result = test_func()
            results.append((test_name, result))
        except Exception as e:
            print(f"\n❌ Test '{test_name}' crashed: {e}")
            results.append((test_name, False))
    
    # Summary
    print("\n" + "=" * 60)
    print("SECURITY TEST SUMMARY")
    print("=" * 60)
    
    all_passed = True
    for test_name, passed in results:
        status = "✅ PASSED" if passed else "❌ FAILED"
        print(f"{status}: {test_name}")
        if not passed:
            all_passed = False
    
    print()
    if all_passed:
        print("🎉 ALL SECURITY TESTS PASSED!")
        print("Command injection vulnerabilities appear to be fixed.")
        return 0
    else:
        print("⚠️  SOME SECURITY TESTS FAILED!")
        print("Security vulnerabilities may still exist.")
        return 1

if __name__ == "__main__":
    sys.exit(main())