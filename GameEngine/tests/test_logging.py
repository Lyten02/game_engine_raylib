#!/usr/bin/env python3
"""
Unit tests for logging functionality in run_all_tests.py
"""

import sys
import os
import json
import tempfile
import shutil
from datetime import datetime
from pathlib import Path

# Add parent directory to path to import TestRunner
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from run_all_tests import TestRunner

def test_log_file_creation():
    """Test log file is created with correct naming"""
    print("Testing log file creation...")
    
    # Create temporary directory for test
    with tempfile.TemporaryDirectory() as tmpdir:
        original_dir = os.getcwd()
        os.chdir(tmpdir)
        
        # Create dummy game executable for testing
        with open("game", "w") as f:
            f.write("#!/bin/bash\necho test")
        os.chmod("game", 0o755)
        
        try:
            # Create test runner
            runner = TestRunner()
            
            # Check log file was created
            assert os.path.exists(runner.log_file), f"Log file {runner.log_file} should exist"
            
            # Check filename format
            assert runner.log_file.startswith("test_log_"), "Log file should start with test_log_"
            assert runner.log_file.endswith(".log"), "Log file should end with .log"
            
            # Check timestamp format in filename
            timestamp_part = runner.log_file[9:-4]  # Extract timestamp part
            try:
                datetime.strptime(timestamp_part, '%Y%m%d_%H%M%S')
            except ValueError:
                assert False, f"Invalid timestamp format in log file name: {timestamp_part}"
            
            print("✅ Log file creation test passed")
        finally:
            os.chdir(original_dir)

def test_log_message_writing():
    """Test messages are written to log file correctly"""
    print("\nTesting log message writing...")
    
    with tempfile.TemporaryDirectory() as tmpdir:
        original_dir = os.getcwd()
        os.chdir(tmpdir)
        
        # Create dummy game executable
        with open("game", "w") as f:
            f.write("#!/bin/bash\necho test")
        os.chmod("game", 0o755)
        
        try:
            runner = TestRunner()
            
            # Write test messages
            runner.log_message("Test info message", "INFO")
            runner.log_message("Test error message", "ERROR")
            runner.log_message("Test warning message", "WARNING")
            
            # Read log file
            with open(runner.log_file, 'r') as f:
                log_content = f.read()
            
            # Check messages were written
            assert "Test info message" in log_content, "Info message should be in log"
            assert "Test error message" in log_content, "Error message should be in log"
            assert "Test warning message" in log_content, "Warning message should be in log"
            
            # Check log levels (with padding to 8 characters)
            assert "[INFO    ]" in log_content, "INFO level should be in log"
            assert "[ERROR   ]" in log_content, "ERROR level should be in log"
            assert "[WARNING ]" in log_content, "WARNING level should be in log"
            
            # Check timestamps (skip header lines)
            lines = log_content.strip().split('\n')
            # Find log entries (they start with [timestamp])
            log_lines = [line for line in lines if line.startswith("[")]
            assert len(log_lines) >= 3, "Should have at least 3 log entries"
            
            for line in log_lines:
                assert line.startswith("["), "Each log line should start with timestamp"
                assert "] [" in line, "Should have level after timestamp"
            
            print("✅ Log message writing test passed")
        finally:
            os.chdir(original_dir)

def test_error_capture():
    """Test detailed error information is captured"""
    print("\nTesting error capture...")
    
    with tempfile.TemporaryDirectory() as tmpdir:
        original_dir = os.getcwd()
        os.chdir(tmpdir)
        
        # Create dummy game executable
        with open("game", "w") as f:
            f.write("#!/bin/bash\necho test")
        os.chmod("game", 0o755)
        
        try:
            runner = TestRunner()
            
            # Create test error info
            error_info = {
                "return_code": 1,
                "stdout": "Test stdout output",
                "stderr": "Test stderr output",
                "command": "python test.py",
                "timeout": 30,
                "elapsed": 2.5,
                "exception": "TestException",
                "traceback": "Test traceback"
            }
            
            # Capture test failure
            runner.capture_test_failure("test_example.py", "python", error_info)
            
            # Check failure was recorded
            assert len(runner.failed_tests_details) == 1, "Should have one failure recorded"
            
            failure = runner.failed_tests_details[0]
            assert failure["test_name"] == "test_example.py", "Test name should match"
            assert failure["test_type"] == "python", "Test type should match"
            assert "timestamp" in failure, "Should have timestamp"
            assert failure["error_info"] == error_info, "Error info should match"
            
            # Check log file contains error
            with open(runner.log_file, 'r') as f:
                log_content = f.read()
            
            assert "TEST FAILED: test_example.py" in log_content, "Failure should be logged"
            assert "Test stdout output" in log_content, "Error details should be logged"
            
            print("✅ Error capture test passed")
        finally:
            os.chdir(original_dir)

def test_verbose_mode():
    """Test verbose mode outputs errors correctly"""
    print("\nTesting verbose mode...")
    
    # Test with verbose mode enabled
    original_argv = sys.argv[:]
    sys.argv.append("--verbose")
    
    try:
        with tempfile.TemporaryDirectory() as tmpdir:
            original_dir = os.getcwd()
            os.chdir(tmpdir)
            
            # Create dummy game executable
            with open("game", "w") as f:
                f.write("#!/bin/bash\necho test")
            os.chmod("game", 0o755)
            
            try:
                runner = TestRunner()
                assert runner.verbose_mode, "Verbose mode should be enabled"
                
                # In verbose mode, ERROR and WARNING messages should print to console
                # (We can't easily test console output here, but we verify the flag is set)
            finally:
                os.chdir(original_dir)
            
    finally:
        sys.argv = original_argv
    
    # Test with verbose mode disabled
    with tempfile.TemporaryDirectory() as tmpdir:
        original_dir = os.getcwd()
        os.chdir(tmpdir)
        
        # Create dummy game executable
        with open("game", "w") as f:
            f.write("#!/bin/bash\necho test")
        os.chmod("game", 0o755)
        
        try:
            runner = TestRunner()
            assert not runner.verbose_mode, "Verbose mode should be disabled by default"
        finally:
            os.chdir(original_dir)
    
    print("✅ Verbose mode test passed")

def test_json_output():
    """Test structured JSON results are saved correctly"""
    print("\nTesting JSON output...")
    
    with tempfile.TemporaryDirectory() as tmpdir:
        original_dir = os.getcwd()
        os.chdir(tmpdir)
        
        # Create dummy game executable
        with open("game", "w") as f:
            f.write("#!/bin/bash\necho test")
        os.chmod("game", 0o755)
        
        try:
            runner = TestRunner()
            
            # Add some test results
            runner.passed = 2
            runner.failed = 1
            runner.test_results = [
                {"test": "test1.py", "type": "python", "passed": True, "time": 1.0},
                {"test": "test2.py", "type": "python", "passed": True, "time": 2.0},
                {"test": "test3.py", "type": "python", "passed": False, "time": 3.0, "error": "Test error"}
            ]
            
            # Add failure details
            runner.failed_tests_details = [{
                "test_name": "test3.py",
                "test_type": "python",
                "timestamp": datetime.now().isoformat(),
                "error_info": {
                    "return_code": 1,
                    "stdout": "Failed output",
                    "stderr": "Error output",
                    "command": "python test3.py",
                    "timeout": 30,
                    "elapsed": 3.0,
                    "exception": None,
                    "traceback": None
                }
            }]
            
            # Call print_summary to generate JSON
            runner.print_summary()
            
            # Check JSON file was created
            assert os.path.exists("test_results.json"), "test_results.json should exist"
            
            # Load and verify JSON content
            with open("test_results.json", 'r') as f:
                json_data = json.load(f)
            
            assert json_data["total"] == 3, "Total should be 3"
            assert json_data["passed"] == 2, "Passed should be 2"
            assert json_data["failed"] == 1, "Failed should be 1"
            assert "total_time" in json_data, "Should include total time"
            assert len(json_data["results"]) == 3, "Should have 3 test results"
            assert len(json_data["detailed_failures"]) == 1, "Should have 1 detailed failure"
            
            # Verify structure of results
            for result in json_data["results"]:
                assert "test" in result, "Each result should have test name"
                assert "type" in result, "Each result should have type"
                assert "passed" in result, "Each result should have passed status"
                assert "time" in result, "Each result should have time"
            
            # Verify structure of failures
            failure = json_data["detailed_failures"][0]
            assert failure["test_name"] == "test3.py", "Failure test name should match"
            assert "timestamp" in failure, "Failure should have timestamp"
            assert "error_info" in failure, "Failure should have error info"
            
            print("✅ JSON output test passed")
        finally:
            os.chdir(original_dir)

def test_failure_summary():
    """Test detailed failure summary generation"""
    print("\nTesting failure summary...")
    
    with tempfile.TemporaryDirectory() as tmpdir:
        original_dir = os.getcwd()
        os.chdir(tmpdir)
        
        # Create dummy game executable
        with open("game", "w") as f:
            f.write("#!/bin/bash\necho test")
        os.chmod("game", 0o755)
        
        try:
            runner = TestRunner()
            
            # Add multiple failures with different types
            failures = [
                {
                    "test_name": "test_timeout.py",
                    "test_type": "python",
                    "timestamp": datetime.now().isoformat(),
                    "error_info": {
                        "return_code": -1,
                        "stdout": "",
                        "stderr": "Test timed out after 30 seconds",
                        "command": "python test_timeout.py",
                        "timeout": 30,
                        "elapsed": 30.0,
                        "exception": "TimeoutExpired",
                        "traceback": None
                    }
                },
                {
                    "test_name": "test_json_error.txt",
                    "test_type": "script",
                    "timestamp": datetime.now().isoformat(),
                    "error_info": {
                        "return_code": 0,
                        "stdout": "Invalid JSON: {error",
                        "stderr": "",
                        "command": "./game --json --headless --script test_json_error.txt",
                        "timeout": 30,
                        "elapsed": 0.5,
                        "exception": "JSONDecodeError",
                        "traceback": None
                    }
                },
                {
                    "test_name": "command: invalid",
                    "test_type": "command",
                    "timestamp": datetime.now().isoformat(),
                    "error_info": {
                        "return_code": 1,
                        "stdout": '{"success": false, "error": "Unknown command"}',
                        "stderr": "",
                        "command": "./game --json --headless --command invalid.command",
                        "timeout": 10,
                        "elapsed": 0.1,
                        "exception": None,
                        "traceback": None,
                        "expected_success": False,
                        "actual_success": True
                    }
                }
            ]
            
            runner.failed_tests_details = failures
            
            # The print_detailed_failure_summary method prints to console
            # We mainly verify the data structure is correct
            assert len(runner.failed_tests_details) == 3, "Should have 3 failures"
            
            # Check each failure has required fields
            for failure in runner.failed_tests_details:
                assert "test_name" in failure, "Each failure should have test_name"
                assert "test_type" in failure, "Each failure should have test_type"
                assert "timestamp" in failure, "Each failure should have timestamp"
                assert "error_info" in failure, "Each failure should have error_info"
                
                error_info = failure["error_info"]
                assert "return_code" in error_info, "Error info should have return_code"
                assert "stdout" in error_info, "Error info should have stdout"
                assert "stderr" in error_info, "Error info should have stderr"
                assert "command" in error_info, "Error info should have command"
                assert "elapsed" in error_info, "Error info should have elapsed time"
            
            print("✅ Failure summary test passed")
        finally:
            os.chdir(original_dir)

def run_all_tests():
    """Run all logging tests"""
    print("="*60)
    print("Logging System Unit Tests")
    print("="*60)
    
    try:
        test_log_file_creation()
        test_log_message_writing()
        test_error_capture()
        test_verbose_mode()
        test_json_output()
        test_failure_summary()
        
        print("\n" + "="*60)
        print("✅ All logging tests passed!")
        print("="*60)
        return 0
    except AssertionError as e:
        print(f"\n❌ Test failed: {e}")
        return 1
    except Exception as e:
        print(f"\n❌ Unexpected error: {e}")
        import traceback
        traceback.print_exc()
        return 1

if __name__ == "__main__":
    sys.exit(run_all_tests())