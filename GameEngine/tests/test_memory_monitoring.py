#!/usr/bin/env python3
"""Test memory monitoring to detect leaks in test execution"""

import subprocess
import time
import os
import sys
import json
import resource
from datetime import datetime

def get_memory_usage():
    """Get current memory usage in MB using resource module"""
    usage = resource.getrusage(resource.RUSAGE_SELF)
    # Return RSS (Resident Set Size) in MB
    return usage.ru_maxrss / 1024 / 1024 if sys.platform == 'linux' else usage.ru_maxrss / 1024 / 1024 / 1024

def monitor_test_execution(test_command, test_name):
    """Monitor memory usage during test execution"""
    print(f"\n🔍 Monitoring memory for: {test_name}")
    
    # Initial memory
    initial_memory = get_memory_usage()
    print(f"Initial memory: {initial_memory:.1f} MB")
    
    # Start test process
    start_time = time.time()
    process = subprocess.Popen(
        test_command,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    
    # Monitor memory during execution
    peak_memory = initial_memory
    memory_samples = []
    
    while process.poll() is None:
        current_memory = get_memory_usage()
        memory_samples.append(current_memory)
        peak_memory = max(peak_memory, current_memory)
        time.sleep(0.1)  # Sample every 100ms
    
    # Get final results
    stdout, stderr = process.communicate()
    end_time = time.time()
    final_memory = get_memory_usage()
    
    # Calculate metrics
    duration = end_time - start_time
    memory_increase = final_memory - initial_memory
    peak_increase = peak_memory - initial_memory
    
    # Analyze for potential leaks
    leak_detected = False
    if memory_increase > 50:  # More than 50MB increase after test
        leak_detected = True
    
    result = {
        "test": test_name,
        "duration": round(duration, 2),
        "initial_memory": round(initial_memory, 1),
        "final_memory": round(final_memory, 1),
        "peak_memory": round(peak_memory, 1),
        "memory_increase": round(memory_increase, 1),
        "peak_increase": round(peak_increase, 1),
        "leak_detected": leak_detected,
        "exit_code": process.returncode,
        "success": process.returncode == 0 and not leak_detected
    }
    
    # Print summary
    print(f"Duration: {duration:.2f}s")
    print(f"Memory: {initial_memory:.1f} MB → {final_memory:.1f} MB (Δ{memory_increase:+.1f} MB)")
    print(f"Peak: {peak_memory:.1f} MB (Δ{peak_increase:+.1f} MB)")
    
    if leak_detected:
        print(f"⚠️  POTENTIAL MEMORY LEAK DETECTED!")
    
    if process.returncode != 0:
        print(f"❌ Test failed with exit code: {process.returncode}")
        if stderr:
            print(f"Error: {stderr[:200]}...")
    
    return result

def test_heavy_tests_memory():
    """Test memory usage of known heavy tests"""
    results = []
    
    # Change to build directory
    build_dir = os.path.join(os.path.dirname(__file__), "../build")
    os.chdir(build_dir)
    
    # Test C++ threading test
    if os.path.exists("tests/test_resource_manager_threading"):
        result = monitor_test_execution(
            ["tests/test_resource_manager_threading"],
            "test_resource_manager_threading (C++)"
        )
        results.append(result)
    
    # Test Python threading test
    result = monitor_test_execution(
        ["python3", "../tests/test_resource_manager_threading.py"],
        "test_resource_manager_threading.py"
    )
    results.append(result)
    
    # Test scene memory safety
    result = monitor_test_execution(
        ["python3", "../tests/test_scene_memory_safety.py"],
        "test_scene_memory_safety.py"
    )
    results.append(result)
    
    # Test parallel runner with limited tests
    result = monitor_test_execution(
        ["python3", "../tests/parallel_test_runner.py", "--workers", "2", "--category", "HEAVY"],
        "parallel_test_runner (HEAVY category)"
    )
    results.append(result)
    
    return results

def test_memory_cleanup_between_tests():
    """Test that memory is properly cleaned between test runs"""
    print("\n🧹 Testing memory cleanup between tests...")
    
    build_dir = os.path.join(os.path.dirname(__file__), "../build")
    os.chdir(build_dir)
    
    initial_memory = get_memory_usage()
    print(f"Initial memory: {initial_memory:.1f} MB")
    
    # Run the same test multiple times
    memory_readings = [initial_memory]
    
    for i in range(5):
        print(f"\nIteration {i+1}/5")
        
        # Run a moderately heavy test
        result = subprocess.run(
            ["python3", "../tests/test_cli_basic.py"],
            capture_output=True,
            text=True,
            timeout=30
        )
        
        current_memory = get_memory_usage()
        memory_readings.append(current_memory)
        print(f"Memory after iteration {i+1}: {current_memory:.1f} MB")
        
        if result.returncode != 0:
            print(f"Test failed on iteration {i+1}")
            return False
    
    # Check for memory growth pattern
    memory_growth = memory_readings[-1] - memory_readings[0]
    avg_growth_per_iteration = memory_growth / 5
    
    print(f"\nTotal memory growth: {memory_growth:.1f} MB")
    print(f"Average growth per iteration: {avg_growth_per_iteration:.1f} MB")
    
    if avg_growth_per_iteration > 10:  # More than 10MB per iteration
        print("❌ Excessive memory growth detected!")
        return False
    
    print("✅ Memory cleanup test passed")
    return True

def generate_memory_report(results):
    """Generate a memory usage report"""
    report = {
        "timestamp": datetime.now().isoformat(),
        "total_tests": len(results),
        "tests_with_leaks": sum(1 for r in results if r["leak_detected"]),
        "failed_tests": sum(1 for r in results if r["exit_code"] != 0),
        "total_peak_memory": max(r["peak_memory"] for r in results),
        "results": results
    }
    
    # Save report
    report_path = "memory_test_report.json"
    with open(report_path, "w") as f:
        json.dump(report, f, indent=2)
    
    print(f"\n📊 Memory report saved to: {report_path}")
    
    # Print summary
    print("\n=== MEMORY TEST SUMMARY ===")
    print(f"Total tests monitored: {report['total_tests']}")
    print(f"Tests with potential leaks: {report['tests_with_leaks']}")
    print(f"Failed tests: {report['failed_tests']}")
    print(f"Peak memory usage: {report['total_peak_memory']:.1f} MB")
    
    if report['tests_with_leaks'] > 0:
        print("\n⚠️  MEMORY LEAKS DETECTED IN:")
        for result in results:
            if result["leak_detected"]:
                print(f"  - {result['test']} (Δ{result['memory_increase']:+.1f} MB)")
    
    return report['tests_with_leaks'] == 0 and report['failed_tests'] == 0

if __name__ == "__main__":
    print("=== MEMORY LEAK DETECTION TEST ===")
    print("Monitoring memory usage during test execution...\n")
    
    # Check if we can find the build directory
    build_path = os.path.join(os.path.dirname(__file__), "../build")
    if not os.path.exists(build_path):
        print("ERROR: Build directory not found!")
        print("Please build the project first.")
        sys.exit(1)
    
    try:
        # Run memory tests
        results = test_heavy_tests_memory()
        
        # Test cleanup
        cleanup_ok = test_memory_cleanup_between_tests()
        
        # Generate report
        all_ok = generate_memory_report(results) and cleanup_ok
        
        if all_ok:
            print("\n✅ All memory tests passed!")
            sys.exit(0)
        else:
            print("\n❌ Memory issues detected!")
            sys.exit(1)
            
    except Exception as e:
        print(f"\n❌ Test failed with error: {e}")
        sys.exit(1)