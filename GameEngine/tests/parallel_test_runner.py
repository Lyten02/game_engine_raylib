#!/usr/bin/env python3
"""
Parallel test execution system for GameEngine tests
"""

import multiprocessing as mp
import subprocess
import time
import os
import json
import traceback
from pathlib import Path
from datetime import datetime
from typing import List, Dict, Tuple, Optional, Any
from concurrent.futures import ProcessPoolExecutor, as_completed
import threading
from queue import Queue

from test_categories import TestCategorizer, TestCategory

class TestResult:
    """Represents the result of a single test execution"""
    def __init__(self, test_name: str, test_type: str, success: bool, 
                 elapsed: float, output: str = "", error: str = "", 
                 return_code: int = 0, worker_id: int = 0):
        self.test_name = test_name
        self.test_type = test_type
        self.success = success
        self.elapsed = elapsed
        self.output = output
        self.error = error
        self.return_code = return_code
        self.worker_id = worker_id
        self.timestamp = datetime.now().isoformat()

    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary for JSON serialization"""
        return {
            "test_name": self.test_name,
            "test_type": self.test_type,
            "success": self.success,
            "elapsed": self.elapsed,
            "output": self.output,
            "error": self.error,
            "return_code": self.return_code,
            "worker_id": self.worker_id,
            "timestamp": self.timestamp
        }

def run_python_test_worker(test_info: Tuple[Path, int, Dict]) -> TestResult:
    """Worker function for running Python tests in parallel"""
    test_path, worker_id, config = test_info
    test_name = test_path.name
    
    start_time = time.time()
    
    try:
        # Determine timeout based on test type
        timeout = 30  # Default timeout
        if "build" in test_name.lower():
            timeout = 180
        elif "resource_manager_memory" in test_name:
            timeout = 120
        elif "full_workflow" in test_name:
            timeout = 180
        elif "config_stress" in test_name or "config_edge_cases" in test_name:
            timeout = 60
        
        # Add skip-full-build flag if requested
        cmd = [config["python_executable"], str(test_path)]
        if config.get("skip_full_build") and any(x in test_name for x in ["build_system", "build_templates", "editor_mode_build", "full_workflow"]):
            cmd.append("--skip-full-build")
        
        # Run the test
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=timeout,
            cwd=config["working_dir"]
        )
        
        elapsed = time.time() - start_time
        success = result.returncode == 0
        
        return TestResult(
            test_name=test_name,
            test_type="python",
            success=success,
            elapsed=elapsed,
            output=result.stdout,
            error=result.stderr,
            return_code=result.returncode,
            worker_id=worker_id
        )
        
    except subprocess.TimeoutExpired:
        elapsed = time.time() - start_time
        return TestResult(
            test_name=test_name,
            test_type="python", 
            success=False,
            elapsed=elapsed,
            error=f"Test timed out after {timeout} seconds",
            return_code=-1,
            worker_id=worker_id
        )
    except Exception as e:
        elapsed = time.time() - start_time
        return TestResult(
            test_name=test_name,
            test_type="python",
            success=False,
            elapsed=elapsed,
            error=f"Exception: {str(e)}\n{traceback.format_exc()}",
            return_code=-2,
            worker_id=worker_id
        )

def run_script_test_worker(test_info: Tuple[Path, int, Dict]) -> TestResult:
    """Worker function for running script tests in parallel"""
    test_path, worker_id, config = test_info
    test_name = test_path.name
    
    start_time = time.time()
    
    try:
        cmd = [config["game_executable"], "--json", "--headless", "--script", str(test_path)]
        
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=30,
            cwd=config["working_dir"]
        )
        
        elapsed = time.time() - start_time
        
        # Parse JSON result for script tests
        try:
            json_result = json.loads(result.stdout)
            success = json_result.get("success", False)
        except (json.JSONDecodeError, KeyError):
            success = result.returncode == 0
        
        return TestResult(
            test_name=test_name,
            test_type="script",
            success=success,
            elapsed=elapsed,
            output=result.stdout,
            error=result.stderr,
            return_code=result.returncode,
            worker_id=worker_id
        )
        
    except subprocess.TimeoutExpired:
        elapsed = time.time() - start_time
        return TestResult(
            test_name=test_name,
            test_type="script",
            success=False,
            elapsed=elapsed,
            error="Test timed out after 30 seconds",
            return_code=-1,
            worker_id=worker_id
        )
    except Exception as e:
        elapsed = time.time() - start_time
        return TestResult(
            test_name=test_name,
            test_type="script",
            success=False,
            elapsed=elapsed,
            error=f"Exception: {str(e)}",
            return_code=-2,
            worker_id=worker_id
        )

def run_command_test_worker(test_info: Tuple[Tuple, int, Dict]) -> TestResult:
    """Worker function for running command tests in parallel"""
    command_info, worker_id, config = test_info
    
    if len(command_info) == 3:
        name, command, expected_success = command_info
    else:
        name, command = command_info
        expected_success = True
    
    start_time = time.time()
    
    try:
        cmd = [config["game_executable"], "--json", "--headless", "--command", command]
        
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=10,
            cwd=config["working_dir"]
        )
        
        elapsed = time.time() - start_time
        
        # Parse JSON result
        try:
            json_result = json.loads(result.stdout)
            actual_success = json_result.get("success", False)
            success = actual_success == expected_success
        except (json.JSONDecodeError, KeyError):
            success = False
        
        return TestResult(
            test_name=f"cmd: {name}",
            test_type="command",
            success=success,
            elapsed=elapsed,
            output=result.stdout,
            error=result.stderr,
            return_code=result.returncode,
            worker_id=worker_id
        )
        
    except subprocess.TimeoutExpired:
        elapsed = time.time() - start_time
        return TestResult(
            test_name=f"cmd: {name}",
            test_type="command",
            success=False,
            elapsed=elapsed,
            error="Command timed out after 10 seconds",
            return_code=-1,
            worker_id=worker_id
        )
    except Exception as e:
        elapsed = time.time() - start_time
        return TestResult(
            test_name=f"cmd: {name}",
            test_type="command",
            success=False,
            elapsed=elapsed,
            error=f"Exception: {str(e)}",
            return_code=-2,
            worker_id=worker_id
        )

class ParallelTestRunner:
    """Parallel test execution runner"""
    
    def __init__(self, test_dir: Path, game_executable: str, skip_full_build: bool = False):
        self.test_dir = test_dir
        self.game_executable = game_executable
        self.skip_full_build = skip_full_build
        self.categorizer = TestCategorizer()
        self.max_workers_override = None
        
        # Results tracking
        self.all_results: List[TestResult] = []
        self.start_time = time.time()
        
        # Configuration for workers
        self.worker_config = {
            "game_executable": game_executable,
            "working_dir": str(test_dir.parent / "build"),  # Tests run from build dir
            "python_executable": "python3",
            "skip_full_build": skip_full_build
        }
        
        # Progress tracking
        self.progress_queue = Queue()
        self.progress_thread = None
        self.total_tests = 0
        self.completed_tests = 0
        self.progress_lock = threading.Lock()
    
    def start_progress_monitor(self):
        """Start background progress monitoring thread"""
        self.progress_thread = threading.Thread(target=self._progress_monitor, daemon=True)
        self.progress_thread.start()
    
    def _progress_monitor(self):
        """Background thread for progress monitoring"""
        while True:
            try:
                # Non-blocking check for updates
                if not self.progress_queue.empty():
                    update = self.progress_queue.get_nowait()
                    if update == "STOP":
                        break
                    self._update_progress(update)
                time.sleep(0.1)
            except:
                continue
    
    def _update_progress(self, result: TestResult):
        """Update progress display"""
        with self.progress_lock:
            self.completed_tests += 1
            
            # Calculate progress
            percent = (self.completed_tests / self.total_tests) * 100 if self.total_tests > 0 else 0
            filled = int(percent / 4)  # 25 chars for 100%
            bar = '█' * filled + '░' * (25 - filled)
            
            # Status icon
            icon = "✅" if result.success else "❌"
            
            # Progress line
            line = f"\r{icon} [{bar}] {percent:5.1f}% ({self.completed_tests}/{self.total_tests}) {result.test_name:<35} ({result.elapsed:.1f}s) [W{result.worker_id}]"
            print(line + " " * 10, end="", flush=True)
    
    def run_test_group(self, group_name: str, tests: List, worker_function, max_workers: int) -> List[TestResult]:
        """Run a group of tests in parallel"""
        if not tests:
            return []
        
        # Apply worker override if set
        if self.max_workers_override and max_workers > 1:
            max_workers = min(max_workers, self.max_workers_override)
        
        print(f"\n🔄 Running {group_name} ({len(tests)} tests, {max_workers} workers)")
        
        # Prepare test info for workers
        test_infos = []
        for i, test in enumerate(tests):
            worker_id = (i % max_workers) + 1
            test_infos.append((test, worker_id, self.worker_config))
        
        results = []
        
        if max_workers == 1:
            # Sequential execution for heavy tests
            for test_info in test_infos:
                result = worker_function(test_info)
                results.append(result)
                self.progress_queue.put(result)
        else:
            # Parallel execution
            with ProcessPoolExecutor(max_workers=max_workers) as executor:
                future_to_test = {executor.submit(worker_function, test_info): test_info for test_info in test_infos}
                
                for future in as_completed(future_to_test):
                    try:
                        result = future.result()
                        results.append(result)
                        self.progress_queue.put(result)
                    except Exception as e:
                        test_info = future_to_test[future]
                        test_name = str(test_info[0])
                        error_result = TestResult(
                            test_name=test_name,
                            test_type="unknown",
                            success=False,
                            elapsed=0,
                            error=f"Worker exception: {str(e)}",
                            return_code=-3,
                            worker_id=test_info[1]
                        )
                        results.append(error_result)
                        self.progress_queue.put(error_result)
        
        print()  # New line after progress updates
        return results
    
    def run_all_tests(self) -> Dict[str, Any]:
        """Run all tests in parallel groups"""
        print("🚀 Parallel GameEngine Test Execution")
        print("=" * 60)
        
        # Get test groups
        groups = self.categorizer.get_parallel_groups(self.test_dir, self.skip_full_build)
        
        # Calculate total tests
        self.total_tests = sum(len(group["tests"]) for group in groups.values())
        print(f"Total tests: {self.total_tests}")
        
        # Start progress monitoring
        self.start_progress_monitor()
        
        # Execute groups in order
        group_results = {}
        
        # 1. Fast parallel tests (lightweight + script)
        if groups["parallel_fast"]["tests"]:
            # Split into Python and script tests
            python_tests = [t for t in groups["parallel_fast"]["tests"] if isinstance(t, Path) and t.suffix == ".py"]
            script_tests = [t for t in groups["parallel_fast"]["tests"] if isinstance(t, Path) and t.suffix == ".txt"]
            
            if python_tests:
                results = self.run_test_group(
                    "Fast Python Tests", 
                    python_tests, 
                    run_python_test_worker, 
                    groups["parallel_fast"]["max_workers"]
                )
                group_results["parallel_fast_python"] = results
                self.all_results.extend(results)
            
            if script_tests:
                results = self.run_test_group(
                    "Script Tests",
                    script_tests,
                    run_script_test_worker,
                    groups["parallel_fast"]["max_workers"]
                )
                group_results["parallel_fast_script"] = results
                self.all_results.extend(results)
        
        # 2. Build tests with coordination
        if groups["parallel_build"]["tests"]:
            results = self.run_test_group(
                "Build Tests",
                groups["parallel_build"]["tests"],
                run_python_test_worker,
                groups["parallel_build"]["max_workers"]
            )
            group_results["parallel_build"] = results
            self.all_results.extend(results)
        
        # 3. Heavy sequential tests
        if groups["sequential_heavy"]["tests"]:
            results = self.run_test_group(
                "Heavy Tests (Sequential)",
                groups["sequential_heavy"]["tests"],
                run_python_test_worker,
                1
            )
            group_results["sequential_heavy"] = results
            self.all_results.extend(results)
        
        # 4. Command tests
        if groups["commands"]["tests"]:
            results = self.run_test_group(
                "Command Tests",
                groups["commands"]["tests"],
                run_command_test_worker,
                groups["commands"]["max_workers"]
            )
            group_results["commands"] = results
            self.all_results.extend(results)
        
        # Stop progress monitoring
        self.progress_queue.put("STOP")
        if self.progress_thread:
            self.progress_thread.join(timeout=1)
        
        # Generate summary
        return self._generate_summary(group_results)
    
    def _generate_summary(self, group_results: Dict[str, List[TestResult]]) -> Dict[str, Any]:
        """Generate test execution summary"""
        total_time = time.time() - self.start_time
        passed = sum(1 for result in self.all_results if result.success)
        failed = len(self.all_results) - passed
        
        # Convert group results to serializable format
        serializable_group_results = {}
        for group_name, results in group_results.items():
            serializable_group_results[group_name] = [result.to_dict() for result in results]
        
        summary = {
            "total_tests": len(self.all_results),
            "passed": passed,
            "failed": failed,
            "success_rate": (passed / len(self.all_results)) * 100 if self.all_results else 0,
            "total_time": total_time,
            "group_results": serializable_group_results,
            "failed_tests": [result.to_dict() for result in self.all_results if not result.success],
            "all_results": [result.to_dict() for result in self.all_results]
        }
        
        # Print summary
        print(f"\n{'='*80}")
        print("PARALLEL TEST EXECUTION SUMMARY")
        print("="*80)
        print(f"Total tests: {summary['total_tests']}")
        print(f"✅ Passed: {summary['passed']}")
        print(f"❌ Failed: {summary['failed']}")
        print(f"Success rate: {summary['success_rate']:.1f}%")
        print(f"Total time: {summary['total_time']:.1f}s")
        
        if summary['failed_tests']:
            print(f"\n❌ Failed tests:")
            for failed_test in summary['failed_tests']:
                print(f"  - {failed_test['test_name']} ({failed_test['elapsed']:.1f}s)")
                if failed_test['error']:
                    error_preview = failed_test['error'][:100].replace('\n', ' ')
                    print(f"    Error: {error_preview}...")
        
        return summary

def main():
    """Demo the parallel test runner"""
    test_dir = Path(__file__).parent
    game_exe = "./game_engine"
    
    # Find game executable
    possible_paths = ["./game_engine", "../build/game_engine", "build/game_engine"]
    for path in possible_paths:
        if os.path.exists(path):
            game_exe = path
            break
    
    runner = ParallelTestRunner(test_dir, game_exe, skip_full_build=True)
    summary = runner.run_all_tests()
    
    # Save results
    with open("parallel_test_results.json", "w") as f:
        json.dump(summary, f, indent=2, default=str)
    
    print(f"\n📊 Results saved to: parallel_test_results.json")
    
    return summary["failed"] == 0

if __name__ == "__main__":
    import sys
    success = main()
    sys.exit(0 if success else 1)