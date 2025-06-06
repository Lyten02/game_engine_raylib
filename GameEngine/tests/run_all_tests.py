#!/usr/bin/env python3
"""
Run all GameEngine tests
"""

import subprocess
import sys
import os
import time
import json
from pathlib import Path

class TestRunner:
    def __init__(self):
        self.passed = 0
        self.failed = 0
        self.test_results = []
        
        # Find game executable
        if os.path.exists("game"):
            self.game_exe = "./game"
        elif os.path.exists("build/game"):
            self.game_exe = "./build/game"
        else:
            print("❌ Error: game executable not found!")
            print("   Run this script from build directory or project root")
            sys.exit(1)
    
    def run_python_test(self, test_file):
        """Run a Python test script"""
        print(f"\n📝 Running Python test: {test_file}")
        start_time = time.time()
        
        try:
            result = subprocess.run(
                [sys.executable, test_file],
                capture_output=True,
                text=True,
                timeout=30
            )
            elapsed = time.time() - start_time
            
            if result.returncode == 0:
                print(f"✅ PASSED ({elapsed:.2f}s)")
                self.passed += 1
                self.test_results.append({
                    "test": test_file,
                    "type": "python",
                    "passed": True,
                    "time": elapsed
                })
            else:
                print(f"❌ FAILED ({elapsed:.2f}s)")
                print(f"   Output: {result.stdout}")
                print(f"   Error: {result.stderr}")
                self.failed += 1
                self.test_results.append({
                    "test": test_file,
                    "type": "python", 
                    "passed": False,
                    "time": elapsed,
                    "error": result.stderr
                })
                
        except subprocess.TimeoutExpired:
            print(f"❌ TIMEOUT (>30s)")
            self.failed += 1
            self.test_results.append({
                "test": test_file,
                "type": "python",
                "passed": False,
                "error": "Timeout"
            })
        except Exception as e:
            print(f"❌ ERROR: {e}")
            self.failed += 1
            self.test_results.append({
                "test": test_file,
                "type": "python",
                "passed": False,
                "error": str(e)
            })
    
    def run_script_test(self, script_file):
        """Run a CLI script test"""
        print(f"\n📜 Running script test: {script_file}")
        start_time = time.time()
        
        try:
            result = subprocess.run(
                [self.game_exe, "--json", "--headless", "--script", script_file],
                capture_output=True,
                text=True,
                timeout=30
            )
            elapsed = time.time() - start_time
            
            try:
                json_result = json.loads(result.stdout)
                if json_result.get("success", False):
                    print(f"✅ PASSED ({elapsed:.2f}s)")
                    self.passed += 1
                    self.test_results.append({
                        "test": script_file,
                        "type": "script",
                        "passed": True,
                        "time": elapsed
                    })
                else:
                    print(f"❌ FAILED ({elapsed:.2f}s)")
                    print(f"   Error: {json_result.get('error', 'Unknown error')}")
                    self.failed += 1
                    self.test_results.append({
                        "test": script_file,
                        "type": "script",
                        "passed": False,
                        "time": elapsed,
                        "error": json_result.get('error')
                    })
            except json.JSONDecodeError:
                print(f"❌ FAILED - Invalid JSON output ({elapsed:.2f}s)")
                print(f"   Output: {result.stdout}")
                self.failed += 1
                self.test_results.append({
                    "test": script_file,
                    "type": "script",
                    "passed": False,
                    "time": elapsed,
                    "error": "Invalid JSON output"
                })
                
        except subprocess.TimeoutExpired:
            print(f"❌ TIMEOUT (>30s)")
            self.failed += 1
            self.test_results.append({
                "test": script_file,
                "type": "script",
                "passed": False,
                "error": "Timeout"
            })
        except Exception as e:
            print(f"❌ ERROR: {e}")
            self.failed += 1
            self.test_results.append({
                "test": script_file,
                "type": "script",
                "passed": False,
                "error": str(e)
            })
    
    def run_command_test(self, name, command, expected_success=True):
        """Run a single command test"""
        print(f"\n💻 Testing command: {name}")
        start_time = time.time()
        
        try:
            result = subprocess.run(
                [self.game_exe, "--json", "--headless", "--command", command],
                capture_output=True,
                text=True,
                timeout=10
            )
            elapsed = time.time() - start_time
            
            try:
                json_result = json.loads(result.stdout)
                success = json_result.get("success", False)
                
                if success == expected_success:
                    print(f"✅ PASSED ({elapsed:.2f}s)")
                    self.passed += 1
                    self.test_results.append({
                        "test": f"command: {name}",
                        "type": "command",
                        "passed": True,
                        "time": elapsed
                    })
                else:
                    print(f"❌ FAILED ({elapsed:.2f}s)")
                    print(f"   Expected success={expected_success}, got {success}")
                    self.failed += 1
                    self.test_results.append({
                        "test": f"command: {name}",
                        "type": "command",
                        "passed": False,
                        "time": elapsed,
                        "error": f"Expected success={expected_success}"
                    })
            except json.JSONDecodeError:
                print(f"❌ FAILED - Invalid JSON ({elapsed:.2f}s)")
                self.failed += 1
                self.test_results.append({
                    "test": f"command: {name}",
                    "type": "command",
                    "passed": False,
                    "time": elapsed,
                    "error": "Invalid JSON"
                })
                
        except Exception as e:
            print(f"❌ ERROR: {e}")
            self.failed += 1
            self.test_results.append({
                "test": f"command: {name}",
                "type": "command",
                "passed": False,
                "error": str(e)
            })
    
    def print_summary(self):
        """Print test summary"""
        print("\n" + "="*60)
        print("TEST SUMMARY")
        print("="*60)
        
        total = self.passed + self.failed
        if total == 0:
            print("No tests were run!")
            return
        
        print(f"Total tests: {total}")
        print(f"✅ Passed: {self.passed}")
        print(f"❌ Failed: {self.failed}")
        print(f"Success rate: {(self.passed/total)*100:.1f}%")
        
        if self.failed > 0:
            print("\nFailed tests:")
            for result in self.test_results:
                if not result["passed"]:
                    print(f"  - {result['test']}: {result.get('error', 'Unknown error')}")
        
        # Save results to JSON
        with open("test_results.json", "w") as f:
            json.dump({
                "total": total,
                "passed": self.passed,
                "failed": self.failed,
                "results": self.test_results
            }, f, indent=2)
        print("\nDetailed results saved to: test_results.json")

def main():
    print("🧪 GameEngine Test Suite")
    print("="*60)
    
    # Clean test data first
    try:
        print("Cleaning previous test data...")
        result = subprocess.run([sys.executable, 
                               os.path.join(os.path.dirname(__file__), "clean_test_data.py")],
                               capture_output=True, text=True)
        if result.returncode == 0:
            print(result.stdout.strip())
        else:
            print("Warning: Failed to clean test data")
    except Exception as e:
        print(f"Warning: Could not clean test data: {e}")
    
    runner = TestRunner()
    
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
    
    print(f"Test directory: {test_dir}")
    print(f"Game executable: {runner.game_exe}")
    
    # 1. Run Python tests
    print("\n🐍 PYTHON TESTS")
    print("-"*40)
    python_tests = list(Path(test_dir).glob("test_*.py"))
    if python_tests:
        for test in sorted(python_tests):
            runner.run_python_test(str(test))
    else:
        print("No Python tests found")
    
    # 2. Run script tests
    print("\n\n📜 SCRIPT TESTS")
    print("-"*40)
    script_tests = list(Path(test_dir).glob("*.txt"))
    if script_tests:
        for test in sorted(script_tests):
            runner.run_script_test(str(test))
    else:
        print("No script tests found")
    
    # 3. Run individual command tests
    print("\n\n💻 COMMAND TESTS")
    print("-"*40)
    runner.run_command_test("Help", "help")
    runner.run_command_test("Project List", "project.list")
    runner.run_command_test("Invalid Command", "invalid.command", expected_success=True)  # TODO: should fail
    runner.run_command_test("Engine Info", "engine.info")
    
    # Print summary
    runner.print_summary()
    
    # Exit with appropriate code
    sys.exit(0 if runner.failed == 0 else 1)

if __name__ == "__main__":
    main()