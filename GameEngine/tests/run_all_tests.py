#!/usr/bin/env python3
"""
Run all GameEngine tests with enhanced progress bar and detailed logging
"""

import subprocess
import sys
import os
import time
import json
import traceback
from pathlib import Path
from datetime import datetime

class TestRunner:
    def __init__(self):
        self.passed = 0
        self.failed = 0
        self.test_results = []
        self.failed_tests_details = []
        
        # Enhanced functionality
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        self.log_file = f"test_log_{timestamp}.log"
        self.verbose_mode = "--verbose" in sys.argv or "-v" in sys.argv
        self.disable_progress = "--no-progress" in sys.argv
        self.skip_full_build = "--skip-full-build" in sys.argv
        self.current_test = 0
        self.total_tests = 0
        self.start_time = time.time()
        
        # Create the log file
        with open(self.log_file, 'w') as f:
            f.write(f"GameEngine Test Log - {datetime.now().isoformat()}\n")
            f.write("="*80 + "\n")
        
        # Find game executable
        if os.path.exists("game"):
            self.game_exe = "./game"
        elif os.path.exists("build/game"):
            self.game_exe = "./build/game"
        else:
            print("❌ Error: game executable not found!")
            print("   Run this script from build directory or project root")
            sys.exit(1)
    
    def log_message(self, message, level="INFO"):
        """Write message to log file with timestamp"""
        timestamp = datetime.now().strftime('%H:%M:%S.%f')[:-3]
        log_entry = f"[{timestamp}] [{level}] {message}\n"
        
        with open(self.log_file, 'a') as f:
            f.write(log_entry)
        
        if self.verbose_mode and level in ["ERROR", "WARNING"]:
            print(f"   {message}")
    
    def print_progress(self, current, total, test_name="", status="", elapsed=None):
        """Display progress bar with test information"""
        if self.disable_progress:
            return
            
        # Calculate progress
        percent = (current / total) * 100 if total > 0 else 0
        filled = int(percent / 4)  # 25 chars for 100%
        
        # Build progress bar
        bar = '█' * filled + '░' * (25 - filled)
        
        # Status icon
        icon = {"running": "⏳", "passed": "✅", "failed": "❌", "timeout": "⏱️"}.get(status, "❓")
        
        # Format test name (truncate if too long)
        max_name_len = 30
        if len(test_name) > max_name_len:
            test_name = test_name[:max_name_len-3] + "..."
        
        # Build output line
        elapsed_str = f"({elapsed:.1f}s)" if elapsed else ""
        line = f"\r{icon} [{bar}] {percent:5.1f}% ({current}/{total}) {test_name:<{max_name_len}} {elapsed_str}"
        
        # Print and flush
        sys.stdout.write(line + " " * 10)  # Extra spaces to clear previous line
        sys.stdout.flush()
        
        # Print newline if test completed
        if status in ["passed", "failed", "timeout"]:
            print()  # Move to next line
    
    def capture_test_failure(self, test_name, test_type, error_info):
        """Capture detailed information about test failures"""
        failure_detail = {
            "test_name": test_name,
            "test_type": test_type,
            "timestamp": datetime.now().isoformat(),
            "error_info": error_info
        }
        
        self.failed_tests_details.append(failure_detail)
        
        # Log detailed error
        self.log_message(f"TEST FAILED: {test_name}", "ERROR")
        self.log_message(f"Error details: {json.dumps(error_info, indent=2)}", "ERROR")
    
    def run_python_test(self, test_file):
        """Run a Python test script with enhanced error capture"""
        test_name = os.path.basename(test_file)
        self.current_test += 1
        
        # Show running status
        self.print_progress(self.current_test, self.total_tests, test_name, "running")
        self.log_message(f"Starting Python test: {test_name}")
        
        start_time = time.time()
        
        try:
            # Use longer timeout for build tests and resource manager tests
            timeout = 180 if "build" in test_file else 30
            if "resource_manager_memory" in test_file:
                timeout = 120  # 2 minutes for C++ compilation
            
            # Add --skip-full-build flag if requested
            args = [sys.executable, test_file]
            if self.skip_full_build and ("test_build_system.py" in test_file or "test_build_system_both.py" in test_file):
                args.append("--skip-full-build")
            
            result = subprocess.run(
                args,
                capture_output=True,
                text=True,
                timeout=timeout
            )
            elapsed = time.time() - start_time
            
            if result.returncode == 0:
                self.print_progress(self.current_test, self.total_tests, test_name, "passed", elapsed)
                self.log_message(f"Python test PASSED: {test_name} ({elapsed:.2f}s)")
                self.passed += 1
                self.test_results.append({
                    "test": test_file,
                    "type": "python",
                    "passed": True,
                    "time": elapsed
                })
            else:
                self.print_progress(self.current_test, self.total_tests, test_name, "failed", elapsed)
                self.failed += 1
                
                error_info = {
                    "return_code": result.returncode,
                    "stdout": result.stdout,
                    "stderr": result.stderr,
                    "command": " ".join(args),
                    "timeout": timeout,
                    "elapsed": elapsed,
                    "exception": None,
                    "traceback": None
                }
                
                self.capture_test_failure(test_name, "python", error_info)
                self.test_results.append({
                    "test": test_file,
                    "type": "python", 
                    "passed": False,
                    "time": elapsed,
                    "error": result.stderr or result.stdout
                })
                
        except subprocess.TimeoutExpired:
            elapsed = time.time() - start_time
            self.print_progress(self.current_test, self.total_tests, test_name, "timeout", elapsed)
            self.failed += 1
            
            error_info = {
                "return_code": -1,
                "stdout": "",
                "stderr": f"Test timed out after {timeout} seconds",
                "command": " ".join(args),
                "timeout": timeout,
                "elapsed": elapsed,
                "exception": "TimeoutExpired",
                "traceback": None
            }
            
            self.capture_test_failure(test_name, "python", error_info)
            self.test_results.append({
                "test": test_file,
                "type": "python",
                "passed": False,
                "error": "Timeout"
            })
        except Exception as e:
            elapsed = time.time() - start_time
            self.print_progress(self.current_test, self.total_tests, test_name, "failed", elapsed)
            self.failed += 1
            
            error_info = {
                "return_code": -1,
                "stdout": "",
                "stderr": str(e),
                "command": " ".join(args) if 'args' in locals() else "Unknown",
                "timeout": timeout if 'timeout' in locals() else 30,
                "elapsed": elapsed,
                "exception": type(e).__name__,
                "traceback": traceback.format_exc()
            }
            
            self.capture_test_failure(test_name, "python", error_info)
            self.test_results.append({
                "test": test_file,
                "type": "python",
                "passed": False,
                "error": str(e)
            })
    
    def run_script_test(self, script_file):
        """Run a CLI script test with enhanced error capture"""
        test_name = os.path.basename(script_file)
        self.current_test += 1
        
        # Show running status
        self.print_progress(self.current_test, self.total_tests, test_name, "running")
        self.log_message(f"Starting script test: {test_name}")
        
        start_time = time.time()
        
        try:
            cmd = [self.game_exe, "--json", "--headless", "--script", script_file]
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=30
            )
            elapsed = time.time() - start_time
            
            try:
                json_result = json.loads(result.stdout)
                if json_result.get("success", False):
                    self.print_progress(self.current_test, self.total_tests, test_name, "passed", elapsed)
                    self.log_message(f"Script test PASSED: {test_name} ({elapsed:.2f}s)")
                    self.passed += 1
                    self.test_results.append({
                        "test": script_file,
                        "type": "script",
                        "passed": True,
                        "time": elapsed
                    })
                else:
                    self.print_progress(self.current_test, self.total_tests, test_name, "failed", elapsed)
                    self.failed += 1
                    
                    error_info = {
                        "return_code": result.returncode,
                        "stdout": result.stdout,
                        "stderr": result.stderr,
                        "command": " ".join(cmd),
                        "timeout": 30,
                        "elapsed": elapsed,
                        "exception": None,
                        "traceback": None,
                        "json_error": json_result.get('error', 'Unknown error')
                    }
                    
                    self.capture_test_failure(test_name, "script", error_info)
                    self.test_results.append({
                        "test": script_file,
                        "type": "script",
                        "passed": False,
                        "time": elapsed,
                        "error": json_result.get('error')
                    })
            except json.JSONDecodeError:
                self.print_progress(self.current_test, self.total_tests, test_name, "failed", elapsed)
                self.failed += 1
                
                error_info = {
                    "return_code": result.returncode,
                    "stdout": result.stdout,
                    "stderr": result.stderr,
                    "command": " ".join(cmd),
                    "timeout": 30,
                    "elapsed": elapsed,
                    "exception": "JSONDecodeError",
                    "traceback": None
                }
                
                self.capture_test_failure(test_name, "script", error_info)
                self.test_results.append({
                    "test": script_file,
                    "type": "script",
                    "passed": False,
                    "time": elapsed,
                    "error": "Invalid JSON output"
                })
                
        except subprocess.TimeoutExpired:
            elapsed = time.time() - start_time
            self.print_progress(self.current_test, self.total_tests, test_name, "timeout", elapsed)
            self.failed += 1
            
            error_info = {
                "return_code": -1,
                "stdout": "",
                "stderr": "Script timed out after 30 seconds",
                "command": " ".join(cmd) if 'cmd' in locals() else "Unknown",
                "timeout": 30,
                "elapsed": elapsed,
                "exception": "TimeoutExpired",
                "traceback": None
            }
            
            self.capture_test_failure(test_name, "script", error_info)
            self.test_results.append({
                "test": script_file,
                "type": "script",
                "passed": False,
                "error": "Timeout"
            })
        except Exception as e:
            elapsed = time.time() - start_time
            self.print_progress(self.current_test, self.total_tests, test_name, "failed", elapsed)
            self.failed += 1
            
            error_info = {
                "return_code": -1,
                "stdout": "",
                "stderr": str(e),
                "command": " ".join(cmd) if 'cmd' in locals() else "Unknown",
                "timeout": 30,
                "elapsed": elapsed,
                "exception": type(e).__name__,
                "traceback": traceback.format_exc()
            }
            
            self.capture_test_failure(test_name, "script", error_info)
            self.test_results.append({
                "test": script_file,
                "type": "script",
                "passed": False,
                "error": str(e)
            })
    
    def run_command_test(self, name, command, expected_success=True):
        """Run a single command test with enhanced error capture"""
        self.current_test += 1
        
        # Show running status
        self.print_progress(self.current_test, self.total_tests, f"cmd: {name}", "running")
        self.log_message(f"Starting command test: {name}")
        
        start_time = time.time()
        
        try:
            cmd = [self.game_exe, "--json", "--headless", "--command", command]
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=10
            )
            elapsed = time.time() - start_time
            
            try:
                json_result = json.loads(result.stdout)
                success = json_result.get("success", False)
                
                if success == expected_success:
                    self.print_progress(self.current_test, self.total_tests, f"cmd: {name}", "passed", elapsed)
                    self.log_message(f"Command test PASSED: {name} ({elapsed:.2f}s)")
                    self.passed += 1
                    self.test_results.append({
                        "test": f"command: {name}",
                        "type": "command",
                        "passed": True,
                        "time": elapsed
                    })
                else:
                    self.print_progress(self.current_test, self.total_tests, f"cmd: {name}", "failed", elapsed)
                    self.failed += 1
                    
                    error_info = {
                        "return_code": result.returncode,
                        "stdout": result.stdout,
                        "stderr": result.stderr,
                        "command": " ".join(cmd),
                        "timeout": 10,
                        "elapsed": elapsed,
                        "exception": None,
                        "traceback": None,
                        "expected_success": expected_success,
                        "actual_success": success
                    }
                    
                    self.capture_test_failure(f"command: {name}", "command", error_info)
                    self.test_results.append({
                        "test": f"command: {name}",
                        "type": "command",
                        "passed": False,
                        "time": elapsed,
                        "error": f"Expected success={expected_success}"
                    })
            except json.JSONDecodeError:
                self.print_progress(self.current_test, self.total_tests, f"cmd: {name}", "failed", elapsed)
                self.failed += 1
                
                error_info = {
                    "return_code": result.returncode,
                    "stdout": result.stdout,
                    "stderr": result.stderr,
                    "command": " ".join(cmd),
                    "timeout": 10,
                    "elapsed": elapsed,
                    "exception": "JSONDecodeError",
                    "traceback": None
                }
                
                self.capture_test_failure(f"command: {name}", "command", error_info)
                self.test_results.append({
                    "test": f"command: {name}",
                    "type": "command",
                    "passed": False,
                    "time": elapsed,
                    "error": "Invalid JSON"
                })
                
        except Exception as e:
            elapsed = time.time() - start_time
            self.print_progress(self.current_test, self.total_tests, f"cmd: {name}", "failed", elapsed)
            self.failed += 1
            
            error_info = {
                "return_code": -1,
                "stdout": "",
                "stderr": str(e),
                "command": " ".join(cmd) if 'cmd' in locals() else "Unknown",
                "timeout": 10,
                "elapsed": elapsed,
                "exception": type(e).__name__,
                "traceback": traceback.format_exc()
            }
            
            self.capture_test_failure(f"command: {name}", "command", error_info)
            self.test_results.append({
                "test": f"command: {name}",
                "type": "command",
                "passed": False,
                "error": str(e)
            })
    
    def print_detailed_failure_summary(self):
        """Print comprehensive failure analysis"""
        if not self.failed_tests_details:
            return
            
        print("\n" + "="*80)
        print("🔍 DETAILED FAILURE ANALYSIS")
        print("="*80)
        
        for i, failure in enumerate(self.failed_tests_details, 1):
            print(f"\n❌ FAILURE #{i}: {failure['test_name']}")
            print(f"   Type: {failure['test_type']}")
            print(f"   Time: {failure['timestamp']}")
            
            error_info = failure['error_info']
            print("   Details:")
            
            if error_info.get('return_code') is not None:
                print(f"     Return Code: {error_info['return_code']}")
            
            # Show last 200 chars of stdout/stderr if present
            if error_info.get('stdout'):
                stdout_preview = error_info['stdout'][-200:] if len(error_info['stdout']) > 200 else error_info['stdout']
                if stdout_preview.strip():
                    print(f"     Stdout (last 200 chars): ...{stdout_preview}")
            
            if error_info.get('stderr'):
                stderr_preview = error_info['stderr'][-200:] if len(error_info['stderr']) > 200 else error_info['stderr']
                if stderr_preview.strip():
                    print(f"     Stderr (last 200 chars): ...{stderr_preview}")
            
            if error_info.get('elapsed'):
                print(f"     Duration: {error_info['elapsed']:.2f}s")
            
            if error_info.get('exception'):
                print(f"     Exception: {error_info['exception']}")
            
            if error_info.get('json_error'):
                print(f"     JSON Error: {error_info['json_error']}")
    
    def print_summary(self):
        """Print test summary with enhanced failure details"""
        print("\n" + "="*80)
        print("TEST SUMMARY")
        print("="*80)
        
        total = self.passed + self.failed
        if total == 0:
            print("No tests were run!")
            return
        
        total_time = time.time() - self.start_time
        print(f"Total tests: {total}")
        print(f"✅ Passed: {self.passed}")
        print(f"❌ Failed: {self.failed}")
        print(f"Success rate: {(self.passed/total)*100:.1f}%")
        print(f"Total time: {total_time:.1f}s")
        
        # Show detailed failure analysis
        self.print_detailed_failure_summary()
        
        # Save results to JSON
        with open("test_results.json", "w") as f:
            json.dump({
                "total": total,
                "passed": self.passed,
                "failed": self.failed,
                "total_time": total_time,
                "results": self.test_results,
                "detailed_failures": self.failed_tests_details
            }, f, indent=2)
        
        print(f"\n📋 Full details saved to: {self.log_file}")
        print("📊 Detailed results saved to: test_results.json")
    
    def count_total_tests(self, test_dir):
        """Count total number of tests to run"""
        count = 0
        
        # Python tests
        python_tests = list(Path(test_dir).glob("test_*.py"))
        exclude_tests = ["test_build_system_fixed.py", "test_build_system_simple.py"]
        
        # In fast mode, also exclude slow build tests
        if self.skip_full_build:
            exclude_tests.extend(["test_fast_build.py", "test_full_workflow.py", "test_build_full.py"])
        
        python_tests = [t for t in python_tests if t.name not in exclude_tests]
        count += len(python_tests)
        
        # Script tests
        script_tests = list(Path(test_dir).glob("*.txt"))
        count += len(script_tests)
        
        # Command tests (hardcoded for now)
        count += 4  # help, project.list, invalid.command, engine.info
        
        return count

def main():
    print("🧪 GameEngine Test Suite with Progress & Detailed Logging")
    print("="*80)
    
    # Show command line options
    if "--help" in sys.argv or "-h" in sys.argv:
        print("Usage: python run_all_tests.py [options]")
        print("Options:")
        print("  --verbose, -v     Show real-time error details")
        print("  --no-progress     Disable progress bar")
        print("  --skip-full-build Skip full build tests (faster)")
        print("  --help, -h        Show this help message")
        sys.exit(0)
    
    if "--verbose" in sys.argv or "-v" in sys.argv:
        print("💡 Verbose mode enabled - real-time error details will be shown")
    else:
        print("💡 Use --verbose or -v for real-time error details")
    
    if "--skip-full-build" in sys.argv:
        print("⚡ Fast mode: Skipping full build tests")
    else:
        print("🔨 Full mode: Including all build tests (may take several minutes)")
    
    # Clean test data first - DISABLED to preserve cached dependencies
    # Uncomment if you need to clean test data
    # try:
    #     print("\nCleaning previous test data...")
    #     result = subprocess.run([sys.executable, 
    #                            os.path.join(os.path.dirname(__file__), "clean_test_data.py")],
    #                            capture_output=True, text=True)
    #     if result.returncode == 0:
    #         print(result.stdout.strip())
    #     else:
    #         print("Warning: Failed to clean test data")
    # except Exception as e:
    #     print(f"Warning: Could not clean test data: {e}")
    
    runner = TestRunner()
    
    # Log start
    runner.log_message("Starting test suite execution")
    
    # Change to correct directory
    if os.path.exists("build/game"):
        os.chdir("build")
    
    # Find test directory
    if os.path.exists("../tests"):
        test_dir = "../tests"
    elif os.path.exists("tests"):
        test_dir = "tests"
    else:
        print("❌ Error: tests directory not found!")
        sys.exit(1)
    
    print(f"\nTest directory: {test_dir}")
    print(f"Game executable: {runner.game_exe}")
    
    # Count total tests
    runner.total_tests = runner.count_total_tests(test_dir)
    runner.log_message(f"Found {runner.total_tests} tests to execute")
    
    print(f"\n🚀 Running {runner.total_tests} tests...\n")
    
    # 1. Run Python tests
    python_tests = list(Path(test_dir).glob("test_*.py"))
    exclude_tests = ["test_build_system_fixed.py", "test_build_system_simple.py"]
    
    # In fast mode, also exclude slow build tests
    if runner.skip_full_build:
        exclude_tests.extend(["test_fast_build.py", "test_full_workflow.py", "test_build_full.py"])
    
    python_tests = [t for t in python_tests if t.name not in exclude_tests]
    
    if python_tests:
        for test in sorted(python_tests):
            runner.run_python_test(str(test))
    
    # 2. Run script tests
    script_tests = list(Path(test_dir).glob("*.txt"))
    if script_tests:
        for test in sorted(script_tests):
            runner.run_script_test(str(test))
    
    # 3. Run individual command tests
    runner.run_command_test("Help", "help")
    runner.run_command_test("Project List", "project.list")
    runner.run_command_test("Invalid Command", "invalid.command", expected_success=False)
    runner.run_command_test("Engine Info", "engine.info")
    
    # Print summary
    runner.print_summary()
    
    # Exit with appropriate code
    sys.exit(0 if runner.failed == 0 else 1)

if __name__ == "__main__":
    main()