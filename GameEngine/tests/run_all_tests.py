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

# Import TDD path resolver
try:
    from test_path_resolver import get_path_resolver, find_game_executable
except ImportError:
    # Fallback if path resolver not available
    def find_game_executable():
        paths = ["./game_engine", "../build/game_engine", "build/game_engine"]
        for path in paths:
            if os.path.exists(path) and os.access(path, os.X_OK):
                return os.path.abspath(path)
        return None

class TestRunner:
    def __init__(self):
        self.passed = 0
        self.failed = 0
        self.test_results = []
        self.failed_tests_details = []
        
        # Enhanced functionality
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        
        # Create logs directory if it doesn't exist
        logs_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "logs")
        os.makedirs(logs_dir, exist_ok=True)
        
        self.log_file = os.path.join(logs_dir, f"test_log_{timestamp}.log")
        self.verbose_mode = "--verbose" in sys.argv or "-v" in sys.argv
        self.disable_progress = "--no-progress" in sys.argv
        self.skip_full_build = "--skip-full-build" in sys.argv
        self.dry_run = "--dry-run" in sys.argv
        self.current_test = 0
        self.total_tests = 0
        self.start_time = time.time()
        
        # Create the log file with enhanced header
        try:
            with open(self.log_file, 'w') as f:
                f.write(f"GameEngine Test Suite Execution Log\n")
                f.write("="*80 + "\n")
                f.write(f"Start Time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
                f.write(f"Python Version: {sys.version.split()[0]}\n")
                f.write(f"Platform: {sys.platform}\n")
                f.write(f"Working Directory: {os.getcwd()}\n")
                f.write(f"Skip Full Build: {self.skip_full_build}\n")
                f.write(f"Verbose Mode: {self.verbose_mode}\n")
                f.write("="*80 + "\n\n")
        except Exception as e:
            print(f"Warning: Could not create log file: {e}")
            self.log_file = None
        
        # Find game executable using TDD path resolver
        self.game_exe = find_game_executable()
        if not self.game_exe:
            # Allow dry-run without executable
            if self.dry_run:
                self.game_exe = "game_engine"  # Dummy path for dry-run
            else:
                print("❌ Error: game_engine executable not found!")
                print("   Searched paths: ./game_engine, ../build/game_engine, build/game_engine")
                print("   Run this script from tests directory or build directory")
                sys.exit(1)
    
    def log_message(self, message, level="INFO"):
        """Write message to log file with timestamp"""
        timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
        log_entry = f"[{timestamp}] [{level:<8}] {message}\n"
        
        if self.log_file:
            try:
                with open(self.log_file, 'a') as f:
                    f.write(log_entry)
            except Exception as e:
                # Fallback to stdout if logging fails
                print(f"[LOG ERROR] {e}")
                print(log_entry.strip())
        
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
        
        # Log test start with separator
        self.log_message(f"{'='*60}", "INFO")
        self.log_message(f"TEST START: {test_name} ({self.current_test}/{self.total_tests})", "INFO")
        self.log_message(f"Type: Python Test", "INFO")
        self.log_message(f"File: {test_file}", "INFO")
        self.log_message(f"{'='*60}", "INFO")
        
        # Show running status
        self.print_progress(self.current_test, self.total_tests, test_name, "running")
        
        start_time = time.time()
        
        try:
            # Use longer timeout for build tests and resource manager tests
            timeout = 180 if "build" in test_file else 30
            if "resource_manager_memory" in test_file:
                timeout = 120  # 2 minutes for C++ compilation
            if "full_workflow" in test_file:
                timeout = 180  # 3 minutes for full workflow test
            
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
                self.log_message(f"TEST PASSED: {test_name}", "SUCCESS")
                self.log_message(f"Duration: {elapsed:.2f} seconds", "INFO")
                self.log_message(f"Return Code: 0", "INFO")
                if result.stdout.strip():
                    self.log_message(f"Output Preview: {result.stdout[:200]}{'...' if len(result.stdout) > 200 else ''}", "INFO")
                self.log_message(f"{'='*60}\n", "INFO")
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
                
                # Log detailed failure information
                self.log_message(f"TEST FAILED: {test_name}", "ERROR")
                self.log_message(f"Duration: {elapsed:.2f} seconds", "ERROR")
                self.log_message(f"Return Code: {result.returncode}", "ERROR")
                self.log_message(f"Command: {' '.join(args)}", "ERROR")
                
                if result.stdout.strip():
                    self.log_message(f"{'='*40} STDOUT {'='*40}", "ERROR")
                    self.log_message(result.stdout, "ERROR")
                
                if result.stderr.strip():
                    self.log_message(f"{'='*40} STDERR {'='*40}", "ERROR")
                    self.log_message(result.stderr, "ERROR")
                
                self.log_message(f"{'='*60}\n", "ERROR")
                
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
            
            # Log timeout details
            self.log_message(f"TEST TIMEOUT: {test_name}", "ERROR")
            self.log_message(f"Duration: {elapsed:.2f} seconds (exceeded {timeout}s timeout)", "ERROR")
            self.log_message(f"Command: {' '.join(args)}", "ERROR")
            self.log_message(f"The test was forcefully terminated after {timeout} seconds", "ERROR")
            self.log_message(f"{'='*60}\n", "ERROR")
            
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
            
            # Log exception details
            self.log_message(f"TEST EXCEPTION: {test_name}", "ERROR")
            self.log_message(f"Duration: {elapsed:.2f} seconds", "ERROR")
            self.log_message(f"Exception Type: {type(e).__name__}", "ERROR")
            self.log_message(f"Exception Message: {str(e)}", "ERROR")
            self.log_message(f"{'='*40} TRACEBACK {'='*40}", "ERROR")
            self.log_message(traceback.format_exc(), "ERROR")
            self.log_message(f"{'='*60}\n", "ERROR")
            
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
        
        # Log test start with separator
        self.log_message(f"{'='*60}", "INFO")
        self.log_message(f"TEST START: {test_name} ({self.current_test}/{self.total_tests})", "INFO")
        self.log_message(f"Type: Script Test", "INFO")
        self.log_message(f"File: {script_file}", "INFO")
        self.log_message(f"{'='*60}", "INFO")
        
        # Show running status
        self.print_progress(self.current_test, self.total_tests, test_name, "running")
        
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
        
        # Log test start with separator
        self.log_message(f"{'='*60}", "INFO")
        self.log_message(f"TEST START: Command - {name} ({self.current_test}/{self.total_tests})", "INFO")
        self.log_message(f"Type: Command Test", "INFO")
        self.log_message(f"Command: {command}", "INFO")
        self.log_message(f"Expected Success: {expected_success}", "INFO")
        self.log_message(f"{'='*60}", "INFO")
        
        # Show running status
        self.print_progress(self.current_test, self.total_tests, f"cmd: {name}", "running")
        
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
        
        # Add final summary to log file
        with open(self.log_file, 'a') as f:
            f.write("\n" + "="*80 + "\n")
            f.write("FINAL TEST EXECUTION SUMMARY\n")
            f.write("="*80 + "\n")
            f.write(f"End Time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
            f.write(f"Total Duration: {total_time:.1f} seconds\n")
            f.write(f"Total Tests: {total}\n")
            f.write(f"Passed: {self.passed} ({(self.passed/total)*100:.1f}%)\n")
            f.write(f"Failed: {self.failed} ({(self.failed/total)*100:.1f}%)\n")
            f.write("\n")
            
            # Add execution time table
            f.write("TEST EXECUTION TIME BREAKDOWN\n")
            f.write("-"*80 + "\n")
            f.write(f"{'Test Name':<50} {'Type':<10} {'Status':<10} {'Time (s)':<10}\n")
            f.write("-"*80 + "\n")
            
            # Sort tests by execution time
            sorted_results = sorted(self.test_results, key=lambda x: x.get('time', 0), reverse=True)
            for result in sorted_results:
                test_name = os.path.basename(result['test'])
                if len(test_name) > 47:
                    test_name = test_name[:44] + "..."
                status = "PASSED" if result['passed'] else "FAILED"
                time_str = f"{result.get('time', 0):.2f}" if 'time' in result else "N/A"
                f.write(f"{test_name:<50} {result['type']:<10} {status:<10} {time_str:<10}\n")
            
            f.write("="*80 + "\n")
        
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
        
        # Script tests (exclude CMakeCache.txt and other non-test files)
        script_tests = list(Path(test_dir).glob("*.txt"))
        script_tests = [t for t in script_tests if t.name not in ["CMakeCache.txt", "CMakeLists.txt"]]
        count += len(script_tests)
        
        # Command tests (hardcoded for now)
        count += 4  # help, project.list, invalid.command, engine.info
        
        return count

def cleanup_test_projects(test_dir=".", keep_last=5):
    """Clean up old test projects keeping only the most recent ones"""
    test_projects_dir = os.path.join(test_dir, "test_projects")
    if not os.path.exists(test_projects_dir):
        return 0
    
    import glob
    import shutil
    
    projects = glob.glob(os.path.join(test_projects_dir, "TestProject*"))
    if len(projects) <= keep_last:
        return 0
    
    # Sort by modification time
    projects.sort(key=lambda x: os.path.getmtime(x), reverse=True)
    
    # Remove old projects
    removed = 0
    for project in projects[keep_last:]:
        try:
            shutil.rmtree(project)
            removed += 1
        except Exception:
            pass
    
    return removed

def rotate_logs(logs_dir="logs", keep_days=7, max_files=10):
    """Rotate old log files"""
    if not os.path.exists(logs_dir):
        return 0
    
    import glob
    
    current_time = time.time()
    cutoff_time = current_time - (keep_days * 24 * 60 * 60)
    
    log_files = glob.glob(os.path.join(logs_dir, "test_log_*.log"))
    removed = 0
    
    # Remove old files
    for log_file in log_files:
        try:
            if os.path.getmtime(log_file) < cutoff_time:
                os.unlink(log_file)
                removed += 1
        except Exception:
            pass
    
    # Keep only max_files most recent
    remaining = glob.glob(os.path.join(logs_dir, "test_log_*.log"))
    remaining.sort(key=lambda x: os.path.getmtime(x), reverse=True)
    
    for log_file in remaining[max_files:]:
        try:
            os.unlink(log_file)
            removed += 1
        except Exception:
            pass
    
    return removed

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
        print("  --parallel, -p    Run tests in parallel (experimental)")
        print("  --workers N       Number of parallel workers (default: auto)")
        print("  --dry-run         Test runner without executing tests")
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
    
    # Check for parallel mode
    use_parallel = "--parallel" in sys.argv or "-p" in sys.argv
    if use_parallel:
        # Import and use parallel runner
        from parallel_test_runner import ParallelTestRunner
        
        print("🚀 Parallel execution mode enabled (experimental)")
        
        # Parse workers argument
        max_workers = None
        if "--workers" in sys.argv:
            try:
                idx = sys.argv.index("--workers")
                if idx + 1 < len(sys.argv):
                    max_workers = int(sys.argv[idx + 1])
                    print(f"  Using {max_workers} workers")
            except (ValueError, IndexError):
                print("  Invalid --workers argument, using default")
        
        # Run parallel tests
        test_dir = Path(__file__).parent
        # Use TDD path resolver for consistent executable discovery
        game_exe = find_game_executable()
        if not game_exe:
            print("❌ Error: game_engine executable not found for parallel tests!")
            return
        
        parallel_runner = ParallelTestRunner(
            test_dir,
            game_exe,
            skip_full_build="--skip-full-build" in sys.argv
        )
        
        # Override worker counts if specified
        if max_workers:
            # Need to override in the runner before execution
            parallel_runner.max_workers_override = max_workers
        
        # Run tests
        summary = parallel_runner.run_all_tests()
        
        # Save results in standard format
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        log_file = f"test_log_{timestamp}.log"
        
        with open(log_file, 'w') as f:
            # Enhanced header with system information
            f.write(f"GameEngine Parallel Test Suite Execution Log\n")
            f.write("="*80 + "\n")
            f.write(f"Start Time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
            f.write(f"Test Mode: Parallel Execution\n")
            f.write(f"Python Version: {sys.version.split()[0]}\n")
            f.write(f"Platform: {sys.platform}\n")
            f.write(f"Working Directory: {os.getcwd()}\n")
            f.write(f"Skip Full Build: {'--skip-full-build' in sys.argv}\n")
            f.write(f"Max Workers: {max_workers if max_workers else 'Auto'}\n")
            f.write("="*80 + "\n\n")
            
            # Summary section
            f.write("EXECUTION SUMMARY\n")
            f.write("-"*80 + "\n")
            f.write(f"Total tests: {summary['total_tests']}\n")
            f.write(f"Passed: {summary['passed']}\n")
            f.write(f"Failed: {summary['failed']}\n")
            f.write(f"Success rate: {summary['success_rate']:.1f}%\n")
            f.write(f"Total time: {summary['total_time']:.1f}s\n")
            f.write("="*80 + "\n\n")
            
            # Group results by type
            f.write("TEST RESULTS BY TYPE\n")
            f.write("="*80 + "\n\n")
            
            # Organize results by test type
            results_by_type = {}
            for result_dict in summary.get('all_results', []):
                test_type = result_dict['test_type']
                if test_type not in results_by_type:
                    results_by_type[test_type] = []
                results_by_type[test_type].append(result_dict)
            
            # Write results grouped by type
            for test_type, results in sorted(results_by_type.items()):
                f.write(f"{test_type.upper()} TESTS ({len(results)} tests)\n")
                f.write("-"*60 + "\n")
                
                for result_dict in sorted(results, key=lambda x: x['timestamp']):
                    status = "PASSED" if result_dict['success'] else "FAILED"
                    f.write(f"\n[{result_dict['timestamp']}] {status}: {result_dict['test_name']}\n")
                    f.write(f"  Duration: {result_dict['elapsed']:.2f}s\n")
                    f.write(f"  Worker ID: {result_dict['worker_id']}\n")
                    f.write(f"  Return Code: {result_dict.get('return_code', 'N/A')}\n")
                    
                    if not result_dict['success']:
                        f.write(f"  {'='*40} ERROR DETAILS {'='*40}\n")
                        if result_dict.get('error'):
                            f.write(f"  Error: {result_dict['error']}\n")
                        if result_dict.get('output'):
                            f.write(f"  Output: {result_dict['output'][:500]}{'...' if len(result_dict.get('output', '')) > 500 else ''}\n")
                        f.write(f"  {'='*80}\n")
                f.write("\n")
            
            # Failed tests summary at the end
            if summary.get('failed_tests'):
                f.write("\n" + "="*80 + "\n")
                f.write("FAILED TESTS SUMMARY\n")
                f.write("="*80 + "\n")
                for failed in summary['failed_tests']:
                    f.write(f"\n❌ {failed['test_name']}\n")
                    f.write(f"   Type: {failed['test_type']}\n")
                    f.write(f"   Duration: {failed['elapsed']:.2f}s\n")
                    f.write(f"   Worker: {failed['worker_id']}\n")
                    if failed.get('error'):
                        error_lines = failed['error'].split('\n')
                        f.write(f"   Error: {error_lines[0]}\n")
                        for line in error_lines[1:5]:  # Show first 5 lines
                            if line.strip():
                                f.write(f"          {line}\n")
        
        # Save JSON results
        with open("test_results.json", 'w') as f:
            json.dump(summary, f, indent=2, default=str)
        
        print(f"\n📋 Full details saved to: {log_file}")
        print("📊 Detailed results saved to: test_results.json")
        
        # Exit with appropriate code
        sys.exit(0 if summary['failed'] == 0 else 1)
    
    # Clean up old test projects and logs
    test_dir = os.path.dirname(os.path.abspath(__file__))
    
    # Clean test projects
    removed_projects = cleanup_test_projects(test_dir)
    if removed_projects > 0:
        print(f"🧹 Cleaned up {removed_projects} old test projects")
    
    # Rotate logs
    logs_dir = os.path.join(test_dir, "logs")
    removed_logs = rotate_logs(logs_dir)
    if removed_logs > 0:
        print(f"🧹 Rotated {removed_logs} old log files")
    
    runner = TestRunner()
    
    # Log start
    runner.log_message("Starting test suite execution")
    
    # Determine where we are and set paths accordingly
    current_dir = os.getcwd()
    if current_dir.endswith("/tests"):
        # We're in the tests directory
        test_dir = "."
        build_dir = "../build"
    elif os.path.exists("tests") and os.path.exists("build/game_engine"):
        # We're in the GameEngine root directory
        test_dir = "tests"
        build_dir = "build"
        os.chdir("build")
    elif os.path.exists("../tests") and os.path.exists("game_engine"):
        # We're in the build directory
        test_dir = "../tests"
        build_dir = "."
    else:
        print("❌ Error: Cannot determine directory structure!")
        print(f"   Current directory: {current_dir}")
        sys.exit(1)
    
    print(f"\nTest directory: {test_dir}")
    print(f"Game executable: {runner.game_exe}")
    
    # Count total tests
    runner.total_tests = runner.count_total_tests(test_dir)
    runner.log_message(f"Found {runner.total_tests} tests to execute")
    
    print(f"\n🚀 Running {runner.total_tests} tests...\n")
    
    # Exit early for dry-run
    if runner.dry_run:
        print("🏃 Dry-run mode: Exiting without running tests")
        sys.exit(0)
    
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
    
    # 2. Run script tests (exclude CMakeCache.txt and other non-test files)
    script_tests = list(Path(test_dir).glob("*.txt"))
    script_tests = [t for t in script_tests if t.name not in ["CMakeCache.txt", "CMakeLists.txt"]]
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