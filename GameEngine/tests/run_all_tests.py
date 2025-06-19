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
        
        # Memory monitoring
        self.memory_monitor = "--memory-monitor" in sys.argv
        self.memory_limit_mb = None
        for i, arg in enumerate(sys.argv):
            if arg == "--memory-limit" and i + 1 < len(sys.argv):
                try:
                    self.memory_limit_mb = int(sys.argv[i + 1])
                except ValueError:
                    print(f"Warning: Invalid memory limit value: {sys.argv[i + 1]}")
        
        # Resource usage tracking
        self.resource_stats = {}
        
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
    
    def _extract_tested_features(self, test_name, stdout):
        """Extract what features were tested from test name and output"""
        features = []
        
        # Extract from test name
        if "cli" in test_name:
            features.append("CLI Interface")
        if "build" in test_name:
            features.append("Build System")
        if "config" in test_name:
            features.append("Configuration")
        if "resource" in test_name:
            features.append("Resource Management")
        if "scene" in test_name:
            features.append("Scene System")
        if "project" in test_name:
            features.append("Project Management")
        if "entity" in test_name:
            features.append("Entity System")
        if "security" in test_name:
            features.append("Security")
        if "logging" in test_name or "log" in test_name:
            features.append("Logging System")
        if "headless" in test_name:
            features.append("Headless Mode")
        if "script" in test_name:
            features.append("Script Execution")
        
        # Extract from output if available
        if stdout and "project." in stdout:
            features.append("Project Commands")
        if stdout and "scene." in stdout:
            features.append("Scene Commands")
        if stdout and "entity." in stdout:
            features.append("Entity Commands")
        
        return features[:3]  # Limit to 3 most relevant
    
    def _analyze_failure(self, test_name, return_code, stderr, stdout):
        """Analyze failure and provide detailed diagnosis"""
        analysis = []
        
        # Return code analysis
        if return_code == 1:
            analysis.append("Test assertion failed - logic error in test")
        elif return_code == 2:
            analysis.append("Command line argument error")
        elif return_code == 127:
            analysis.append("Command not found - executable missing")
        elif return_code == 139:
            analysis.append("Segmentation fault - memory access violation")
        elif return_code < 0:
            analysis.append(f"Process killed by signal {abs(return_code)}")
        else:
            analysis.append(f"Unknown error with exit code {return_code}")
        
        # Error message analysis
        if stderr:
            if "ImportError" in stderr or "ModuleNotFoundError" in stderr:
                analysis.append("Python import error - missing dependencies")
            if "FileNotFoundError" in stderr:
                analysis.append("File not found - check paths and file existence")
            if "PermissionError" in stderr:
                analysis.append("Permission denied - check file permissions")
            if "SyntaxError" in stderr:
                analysis.append("Python syntax error in test file")
            if "game_engine" in stderr and "not found" in stderr:
                analysis.append("Game engine executable not built or not in PATH")
            if "Timeout" in stderr or "timeout" in stderr:
                analysis.append("Test timed out - may be hanging or too slow")
            if "CMake" in stderr:
                analysis.append("Build system error - check CMake configuration")
            if "JSON" in stderr or "json" in stderr:
                analysis.append("JSON parsing error - invalid output format")
        
        # Test type specific analysis
        if "build" in test_name and return_code != 0:
            analysis.append("Build test failed - check compiler and dependencies")
        if "cli" in test_name and return_code != 0:
            analysis.append("CLI test failed - check command interface")
        if "headless" in test_name and return_code != 0:
            analysis.append("Headless test failed - check non-GUI execution")
        
        return analysis[:5]  # Limit to 5 most relevant points
    
    def _get_fix_recommendations(self, test_name, return_code, stderr):
        """Provide specific recommendations for fixing the test"""
        recommendations = []
        
        # General recommendations based on error type
        if "game_engine" in stderr and "not found" in stderr:
            recommendations.append("Run 'make' or './rebuild_fast.sh' to build the engine")
            recommendations.append("Check if you're in the correct directory (build/ or GameEngine/)")
        
        if "ImportError" in stderr or "ModuleNotFoundError" in stderr:
            recommendations.append("Install missing Python packages with pip")
            recommendations.append("Check PYTHONPATH environment variable")
        
        if "SyntaxError" in stderr:
            recommendations.append("Fix Python syntax errors in the test file")
            recommendations.append("Check Python version compatibility")
        
        if "Timeout" in stderr or "timeout" in stderr:
            recommendations.append("Increase timeout value in test configuration")
            recommendations.append("Optimize test to run faster")
            recommendations.append("Check for infinite loops or hanging processes")
        
        if "build" in test_name.lower():
            recommendations.append("Ensure all build dependencies are installed")
            recommendations.append("Clean build directory and rebuild from scratch")
            recommendations.append("Check CMake version and configuration")
        
        if "CLI" in stderr or "command" in stderr:
            recommendations.append("Verify CLI command syntax and arguments")
            recommendations.append("Check if JSON output format is enabled")
        
        if return_code == 139:  # Segfault
            recommendations.append("Run test under debugger (gdb) to find crash location")
            recommendations.append("Check for memory leaks with valgrind")
            recommendations.append("Review recent code changes for memory issues")
        
        return recommendations[:4]  # Limit to 4 most actionable recommendations
    
    def _extract_script_commands(self, script_file):
        """Extract commands from script file for logging"""
        commands = []
        try:
            with open(script_file, 'r') as f:
                content = f.read()
                # Extract common command patterns
                lines = content.split('\n')
                for line in lines:
                    line = line.strip()
                    if line and not line.startswith('#'):
                        if any(cmd in line for cmd in ['project.', 'scene.', 'entity.', 'help', 'info']):
                            # Extract just the command name
                            if '.' in line:
                                cmd_part = line.split()[0] if ' ' in line else line
                                commands.append(cmd_part)
                            else:
                                commands.append(line.split()[0] if ' ' in line else line)
        except:
            pass
        
        return list(set(commands))[:5]  # Unique commands, limit to 5
    
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
    
    def _get_memory_usage(self):
        """Get current memory usage in MB"""
        try:
            # Try to use psutil if available
            import psutil
            process = psutil.Process()
            return process.memory_info().rss / 1024 / 1024
        except ImportError:
            # Fallback to resource module
            import resource
            usage = resource.getrusage(resource.RUSAGE_CHILDREN)
            # Different platforms report differently
            if sys.platform == 'darwin':  # macOS
                return usage.ru_maxrss / 1024 / 1024
            else:  # Linux
                return usage.ru_maxrss / 1024
    
    def _cleanup_test_artifacts(self):
        """Clean up test artifacts to prevent memory/disk accumulation"""
        try:
            # Clean up old test projects in output directory
            output_dir = os.path.join(os.path.dirname(self.game_exe), "../output")
            if os.path.exists(output_dir):
                projects = [d for d in os.listdir(output_dir) 
                           if os.path.isdir(os.path.join(output_dir, d)) 
                           and (d.startswith("Test") or d.startswith("MemTest") or d.startswith("SceneTest"))]
                
                # Keep only last 5 projects
                if len(projects) > 5:
                    projects.sort(key=lambda x: os.path.getmtime(os.path.join(output_dir, x)))
                    for project in projects[:-5]:
                        project_path = os.path.join(output_dir, project)
                        try:
                            import shutil
                            shutil.rmtree(project_path)
                            self.log_message(f"Cleaned up old project: {project}", "DEBUG")
                        except:
                            pass
            
            # Force garbage collection
            import gc
            gc.collect()
            
        except Exception as e:
            self.log_message(f"Cleanup error: {e}", "WARNING")
    
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
            args = ['python3', test_file]
            if self.skip_full_build and ("test_build_system.py" in test_file or "test_build_system_both.py" in test_file):
                args.append("--skip-full-build")
            
            # For better error reporting, check syntax first
            syntax_check = subprocess.run(
                ['python3', '-m', 'py_compile', test_file],
                capture_output=True,
                text=True
            )
            
            if syntax_check.returncode != 0:
                # Syntax error - report it directly
                elapsed = 0.05
                self.print_progress(self.current_test, self.total_tests, test_name, "failed", elapsed)
                self.failed += 1
                
                error_info = {
                    "return_code": syntax_check.returncode,
                    "stdout": syntax_check.stdout,
                    "stderr": syntax_check.stderr,
                    "command": " ".join(['python3', '-m', 'py_compile', test_file]),
                    "timeout": timeout,
                    "elapsed": elapsed,
                    "exception": "SyntaxError",
                    "traceback": None
                }
                
                self.capture_test_failure(test_name, "python", error_info)
                self.test_results.append({
                    "test": test_file,
                    "type": "python",
                    "passed": False,
                    "time": elapsed,
                    "error": syntax_check.stderr
                })
                return
            
            # Set up environment with proper PYTHONPATH
            env = os.environ.copy()
            test_dir = os.path.dirname(os.path.abspath(test_file))
            if 'PYTHONPATH' in env:
                env['PYTHONPATH'] = f"{test_dir}{os.pathsep}{env['PYTHONPATH']}"
            else:
                env['PYTHONPATH'] = test_dir
            
            # Check memory before test if monitoring enabled
            if self.memory_monitor:
                initial_memory = self._get_memory_usage()
                self.log_message(f"Memory before test: {initial_memory:.1f} MB", "DEBUG")
            
            # Run from test directory to ensure proper imports
            result = subprocess.run(
                args,
                capture_output=True,
                text=True,
                timeout=timeout,
                cwd=test_dir,
                env=env
            )
            elapsed = time.time() - start_time
            
            # Check memory after test if monitoring enabled
            if self.memory_monitor:
                final_memory = self._get_memory_usage()
                memory_increase = final_memory - initial_memory
                self.log_message(f"Memory after test: {final_memory:.1f} MB (Δ{memory_increase:+.1f} MB)", "DEBUG")
                
                # Check for memory leak
                if memory_increase > 50:  # More than 50MB increase
                    self.log_message(f"⚠️  Potential memory leak detected in {test_name}: {memory_increase:.1f} MB increase", "WARNING")
                
                # Check memory limit
                if self.memory_limit_mb and final_memory > self.memory_limit_mb:
                    self.log_message(f"❌ Memory limit exceeded: {final_memory:.1f} MB > {self.memory_limit_mb} MB", "ERROR")
                    raise MemoryError(f"Memory limit exceeded: {final_memory:.1f} MB")
            
            # Clean up test artifacts after each test
            self._cleanup_test_artifacts()
            
            if result.returncode == 0:
                self.print_progress(self.current_test, self.total_tests, test_name, "passed", elapsed)
                
                # ✅ SUCCESS: Краткий отчет что протестировано
                self.log_message(f"✅ PASSED: {test_name} ({elapsed:.2f}s)", "SUCCESS")
                
                # Краткая информация о том, что было протестировано
                tested_features = self._extract_tested_features(test_name, result.stdout)
                if tested_features:
                    self.log_message(f"   Tested: {', '.join(tested_features)}", "SUCCESS")
                
                self.log_message(f"{'='*60}\n", "SUCCESS")
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
                
                # ❌ FAILURE: Подробные этапы + полная диагностика ошибок
                self.log_message(f"❌ FAILED: {test_name}", "ERROR")
                self.log_message(f"{'='*80}", "ERROR")
                self.log_message(f"FAILURE DIAGNOSIS FOR: {test_name}", "ERROR")
                self.log_message(f"{'='*80}", "ERROR")
                
                # Этап 1: Основная информация
                self.log_message(f"📊 EXECUTION INFO:", "ERROR")
                self.log_message(f"   Duration: {elapsed:.2f} seconds", "ERROR")
                self.log_message(f"   Return Code: {result.returncode}", "ERROR")
                self.log_message(f"   Command: {' '.join(args)}", "ERROR")
                self.log_message(f"   Working Dir: {os.getcwd()}", "ERROR")
                
                # Этап 2: Анализ ошибки
                error_analysis = self._analyze_failure(test_name, result.returncode, result.stderr, result.stdout)
                self.log_message(f"\n🔍 ERROR ANALYSIS:", "ERROR")
                for analysis_point in error_analysis:
                    self.log_message(f"   • {analysis_point}", "ERROR")
                
                # Этап 3: Полный вывод для диагностики
                if result.stdout.strip():
                    self.log_message(f"\n📤 STDOUT (Full Output):", "ERROR")
                    self.log_message(f"{'='*60}", "ERROR")
                    self.log_message(result.stdout, "ERROR")
                    self.log_message(f"{'='*60}", "ERROR")
                
                if result.stderr.strip():
                    self.log_message(f"\n🚨 STDERR (Error Details):", "ERROR")
                    self.log_message(f"{'='*60}", "ERROR")
                    self.log_message(result.stderr, "ERROR")
                    self.log_message(f"{'='*60}", "ERROR")
                
                # Этап 4: Рекомендации по исправлению
                recommendations = self._get_fix_recommendations(test_name, result.returncode, result.stderr)
                if recommendations:
                    self.log_message(f"\n💡 RECOMMENDED FIXES:", "ERROR")
                    for i, rec in enumerate(recommendations, 1):
                        self.log_message(f"   {i}. {rec}", "ERROR")
                
                self.log_message(f"\n{'='*80}\n", "ERROR")
                
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
        except MemoryError as e:
            elapsed = time.time() - start_time
            self.print_progress(self.current_test, self.total_tests, test_name, "failed", elapsed)
            self.failed += 1
            
            # Log memory error
            self.log_message(f"MEMORY LIMIT EXCEEDED: {test_name}", "ERROR")
            self.log_message(f"Duration: {elapsed:.2f} seconds", "ERROR")
            self.log_message(f"Error: {str(e)}", "ERROR")
            self.log_message("Test aborted due to excessive memory usage", "ERROR")
            self.log_message(f"{'='*60}\n", "ERROR")
            
            error_info = {
                "return_code": -2,
                "stdout": "",
                "stderr": str(e),
                "command": " ".join(args),
                "timeout": timeout,
                "elapsed": elapsed,
                "exception": "MemoryError",
                "traceback": None
            }
            
            self.capture_test_failure(test_name, "python", error_info)
            self.test_results.append({
                "test": test_file,
                "type": "python",
                "passed": False,
                "error": "Memory limit exceeded"
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
                    # ✅ SUCCESS: Краткий отчет для script теста
                    self.log_message(f"✅ SCRIPT PASSED: {test_name} ({elapsed:.2f}s)", "SUCCESS")
                    
                    # Краткая информация о командах в скрипте
                    commands = self._extract_script_commands(script_file)
                    if commands:
                        self.log_message(f"   Commands tested: {', '.join(commands[:3])}", "SUCCESS")
                    
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
                    # ✅ SUCCESS: Краткий отчет для command теста
                    self.log_message(f"✅ COMMAND PASSED: {name} ({elapsed:.2f}s)", "SUCCESS")
                    self.log_message(f"   Command: {command}", "SUCCESS")
                    
                    # Показать краткий результат
                    if json_result.get('result'):
                        result_preview = str(json_result['result'])[:100]
                        self.log_message(f"   Result: {result_preview}{'...' if len(str(json_result['result'])) > 100 else ''}", "SUCCESS")
                    
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
        """Print intelligent test summary with success highlights and failure details"""
        print("\n" + "="*80)
        print("📊 INTELLIGENT TEST EXECUTION SUMMARY")
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
        
        # Show successful tests summary (brief)
        if self.passed > 0:
            print(f"\n🎆 SUCCESSFUL TESTS ({self.passed} passed):")
            print("-" * 60)
            successful_tests = [r for r in self.test_results if r['passed']]
            
            # Group by test type
            by_type = {}
            for test in successful_tests:
                test_type = test['type']
                if test_type not in by_type:
                    by_type[test_type] = []
                by_type[test_type].append(test)
            
            for test_type, tests in by_type.items():
                print(f"\n  🟢 {test_type.upper()} Tests ({len(tests)} passed):")
                for test in tests[:5]:  # Show first 5
                    test_name = os.path.basename(test['test'])
                    duration = test.get('time', 0)
                    print(f"    ✅ {test_name:<40} ({duration:.2f}s)")
                if len(tests) > 5:
                    print(f"    ... and {len(tests) - 5} more {test_type} tests")
        
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
        
        # Save results to JSON in the test directory, not build directory
        results_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "test_results.json")
        try:
            with open(results_path, "w") as f:
                json.dump({
                    "total": total,
                    "passed": self.passed,
                    "failed": self.failed,
                    "total_time": total_time,
                    "results": self.test_results,
                    "detailed_failures": self.failed_tests_details
                }, f, indent=2)
        except Exception as e:
            print(f"⚠️  Warning: Could not save test results: {e}")
            # Try to save in current directory as fallback
            try:
                with open("test_results.json", "w") as f:
                    json.dump({
                        "total": total,
                        "passed": self.passed,
                        "failed": self.failed,
                        "total_time": total_time,
                        "results": self.test_results,
                        "detailed_failures": self.failed_tests_details
                    }, f, indent=2)
                results_path = "test_results.json"
            except:
                pass
        
        print(f"\n📋 Full details saved to: {self.log_file}")
        print(f"📊 Detailed results saved to: {results_path}")
    
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
        print("  --dry-run         Test runner without executing tests")
        print("  --memory-monitor  Enable memory leak detection")
        print("  --memory-limit N  Set memory limit in MB (abort if exceeded)")
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
    
    if "--memory-monitor" in sys.argv:
        print("🔍 Memory monitoring enabled - detecting potential leaks")
        
    if "--memory-limit" in sys.argv:
        try:
            idx = sys.argv.index("--memory-limit")
            if idx + 1 < len(sys.argv):
                limit = int(sys.argv[idx + 1])
                print(f"⚠️  Memory limit set to {limit} MB")
        except:
            pass
    
    
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