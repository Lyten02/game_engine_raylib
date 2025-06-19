#!/usr/bin/env python3
"""
TDD RED PHASE: Failing tests for script execution problems
All script tests are failing with FileNotFoundError: './game_engine'
"""

import unittest
import subprocess
import os
import sys
from pathlib import Path

class TestScriptExecutionFix(unittest.TestCase):
    """Tests for script execution that should FAIL initially"""
    
    def setUp(self):
        """Setup test environment"""
        self.original_cwd = os.getcwd()
        
    def tearDown(self):
        """Restore original working directory"""
        os.chdir(self.original_cwd)
    
    def test_game_engine_executable_discovery(self):
        """FAILING: Test that game_engine executable can be found"""
        # This should FAIL because current discovery logic is broken
        
        # Simulate the broken behavior from logs
        expected_paths = [
            "./game_engine",
            "../build/game_engine", 
            "game_engine",
            os.path.join(os.getcwd(), "game_engine")
        ]
        
        found_executable = None
        for path in expected_paths:
            if os.path.exists(path) and os.access(path, os.X_OK):
                found_executable = path
                break
        
        # This should FAIL initially due to path discovery issues
        self.assertIsNotNone(found_executable, "Should find game_engine executable")
        
    def test_script_working_directory_detection(self):
        """FAILING: Test that script tests can determine correct working directory"""
        # This test defines the expected behavior for working directory detection
        
        # The issue: script tests assume they're run from tests/ directory
        # But they're actually run from build/ directory
        
        current_dir = Path.cwd()
        
        # Expected behavior: detect if we're in build/ or tests/ directory
        if current_dir.name == "build":
            # We're in build/, so executable should be at ./game_engine
            expected_executable = "./game_engine"
        elif current_dir.name == "tests":
            # We're in tests/, so executable should be at ../build/game_engine
            expected_executable = "../build/game_engine"
        else:
            # We're somewhere else, need to search
            expected_executable = None
            
        # This should FAIL because the logic doesn't exist yet
        self.assertIsNotNone(expected_executable, "Should determine executable path based on working directory")
        
    def test_script_file_path_resolution(self):
        """FAILING: Test that script files can be found relative to test runner"""
        # The issue: script tests look for files like 'basic_cli_test.txt'
        # But don't know where they are relative to current working directory
        
        script_files = [
            "basic_cli_test.txt",
            "basic_cli_test_safe.txt", 
            "manual_build_test.txt",
            "test_build_batch.txt",
            "test_entities.txt",
            "test_scene_management.txt"
        ]
        
        tests_dir = Path("../tests")  # Assume we're in build/
        if not tests_dir.exists():
            tests_dir = Path(".")  # We're already in tests/
            
        for script_file in script_files:
            script_path = tests_dir / script_file
            # This should FAIL because path resolution logic doesn't exist
            self.assertTrue(script_path.exists(), f"Script file {script_file} should be found")
    
    def test_command_test_execution_path(self):
        """FAILING: Test that command tests can find executable"""
        # Command tests also fail with same executable discovery issue
        
        # Commands being tested:
        commands = [
            "Help",
            "Project List", 
            "Invalid Command",
            "Engine Info"
        ]
        
        # This should FAIL because command execution logic doesn't find executable
        game_exe_path = self._find_game_executable()
        self.assertIsNotNone(game_exe_path, "Command tests should find game_engine executable")
        
        for command in commands:
            # This should FAIL because execution logic is broken
            result = self._execute_command_test(game_exe_path, command)
            self.assertEqual(result.returncode, 0, f"Command '{command}' should execute successfully")
    
    def test_python_import_path_issues(self):
        """FAILING: Test that Python tests can import required modules"""
        # Many Python tests fail with import errors
        
        failing_tests = [
            "test_resource_functionality.py",
            "test_resource_manager_memory.py", 
            "test_resource_manager_threading.py",
            "test_scene_list_only.py",
            "test_scene_memory_safety.py",
            "test_script_runner.py",
            "test_security_cli.py",
            "test_test_system_fixes.py",
            "test_utils.py"
        ]
        
        for test_file in failing_tests:
            # This should FAIL because import path resolution doesn't work
            test_path = Path("../tests") / test_file
            if not test_path.exists():
                test_path = Path(".") / test_file
                
            # Simulate import test
            try:
                # This should FAIL with import errors
                result = subprocess.run([
                    sys.executable, str(test_path)
                ], capture_output=True, text=True, timeout=5)
                
                # This assertion should FAIL initially
                self.assertEqual(result.returncode, 0, f"Test {test_file} should run without import errors")
            except subprocess.TimeoutExpired:
                self.fail(f"Test {test_file} timed out - likely due to import issues")
    
    # Helper methods - now implemented
    def _find_game_executable(self):
        """Find game_engine executable using path resolver"""
        try:
            from test_path_resolver import find_game_executable
            return find_game_executable()
        except ImportError:
            return None
    
    def _execute_command_test(self, executable_path, command):
        """Execute command test with basic implementation"""
        if not executable_path:
            class MockResult:
                returncode = -1
            return MockResult()
            
        try:
            # Basic command execution test
            result = subprocess.run([
                executable_path, "--json", "--headless", "--command", command.lower()
            ], capture_output=True, text=True, timeout=5)
            return result
        except Exception:
            class MockResult:
                returncode = -1
            return MockResult()

if __name__ == "__main__":
    print("🔴 Running FAILING tests for script execution problems...")
    print("These tests define the expected behavior for fixing script execution issues")
    unittest.main(verbosity=2)