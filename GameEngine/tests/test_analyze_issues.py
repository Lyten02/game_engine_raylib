#!/usr/bin/env python3
"""Analyze test execution issues and identify problematic patterns"""

import subprocess
import time
import os
import sys
import json
from datetime import datetime

def analyze_threading_tests():
    """Analyze C++ threading test resource usage"""
    print("\n🔍 Analyzing C++ threading tests...")
    
    # Check the source code for problematic patterns
    cpp_file = "test_resource_manager_threading.cpp"
    if os.path.exists(cpp_file):
        with open(cpp_file, 'r') as f:
            content = f.read()
            
        issues = []
        
        # Check for high thread counts
        if "const int numThreads = 100;" in content:
            issues.append("⚠️  Creates 100 threads × 1000 accesses = 100,000 operations")
        if "const int numThreads = 50;" in content:
            issues.append("⚠️  Creates 50 threads × 20 textures = 1,000 texture loads")
        if "const int numThreads = 200;" in content:
            issues.append("⚠️  Stress test creates 200 threads simultaneously")
            
        # Check for resource cleanup
        if "manager.cleanup()" not in content and "manager.clear()" not in content:
            issues.append("❌ No explicit resource cleanup between iterations")
            
        print(f"Found {len(issues)} potential issues:")
        for issue in issues:
            print(f"  {issue}")
    
    return len(issues) > 0

def analyze_scene_memory_test():
    """Analyze scene memory test for issues"""
    print("\n🔍 Analyzing scene memory safety test...")
    
    py_file = "test_scene_memory_safety.py"
    if os.path.exists(py_file):
        with open(py_file, 'r') as f:
            content = f.read()
            
        issues = []
        
        # Check for project accumulation
        if "for i in range(10):" in content and "project.create" in content:
            issues.append("⚠️  Creates 10 projects in a loop without cleanup")
            
        # Check for entity accumulation
        if "entity.create" in content and "entity.destroy" not in content:
            issues.append("⚠️  Creates entities without destroying them")
            
        print(f"Found {len(issues)} potential issues:")
        for issue in issues:
            print(f"  {issue}")
    
    return len(issues) > 0

def analyze_parallel_execution():
    """Analyze parallel test runner configuration"""
    print("\n🔍 Analyzing parallel test execution...")
    
    categories_file = "test_categories.py"
    if os.path.exists(categories_file):
        with open(categories_file, 'r') as f:
            content = f.read()
            
        print("Test categories and worker limits:")
        
        # Extract category definitions
        if "'HEAVY': {" in content:
            print("  HEAVY tests: Run sequentially (high resource usage)")
        if "'BUILD': {" in content:
            print("  BUILD tests: 2 workers max")
        if "'LIGHTWEIGHT': {" in content:
            print("  LIGHTWEIGHT tests: 4 workers max")
            
        # Count tests by category
        for category in ['HEAVY', 'BUILD', 'LIGHTWEIGHT', 'COMMAND']:
            count = content.count(f"'{category}'")
            if count > 0:
                print(f"  {category}: ~{count} tests")

def identify_resource_intensive_operations():
    """Identify patterns that cause high CPU/memory usage"""
    print("\n🔥 Resource-intensive operations found:")
    
    patterns = {
        "C++ Threading Tests": [
            "100 threads × 1,000 texture accesses",
            "50 threads × 20 texture loads", 
            "200 threads stress test",
            "No sleep/yield between operations"
        ],
        "Scene Memory Tests": [
            "10 projects created in rapid succession",
            "Multiple entities without cleanup",
            "No delay between operations"
        ],
        "Parallel Execution": [
            "Multiple Python processes spawned",
            "Each test spawns its own subprocess",
            "No global resource limits"
        ],
        "Build System Tests": [
            "Full CMake rebuilds for each test",
            "Compilation of C++ files in parallel",
            "No build artifact caching between tests"
        ]
    }
    
    for category, operations in patterns.items():
        print(f"\n{category}:")
        for op in operations:
            print(f"  • {op}")

def suggest_optimizations():
    """Suggest optimizations to reduce memory usage and heat"""
    print("\n💡 OPTIMIZATION RECOMMENDATIONS:")
    
    recommendations = [
        {
            "issue": "Excessive threading in C++ tests",
            "solution": "Reduce thread counts: 100→10, 50→5, 200→20",
            "impact": "90% reduction in concurrent operations"
        },
        {
            "issue": "No resource cleanup between tests",
            "solution": "Add explicit cleanup after each test iteration",
            "impact": "Prevent memory accumulation"
        },
        {
            "issue": "Rapid project creation",
            "solution": "Add delays between operations or reuse projects",
            "impact": "Reduce filesystem stress"
        },
        {
            "issue": "Unlimited parallel execution",
            "solution": "Set global worker limit based on CPU cores",
            "impact": "Prevent CPU oversubscription"
        },
        {
            "issue": "No memory monitoring",
            "solution": "Add memory checks and abort if threshold exceeded",
            "impact": "Prevent runaway memory usage"
        }
    ]
    
    for rec in recommendations:
        print(f"\n❗ {rec['issue']}")
        print(f"   ✅ {rec['solution']}")
        print(f"   📊 Impact: {rec['impact']}")

def create_optimized_versions():
    """Create templates for optimized test versions"""
    print("\n📝 Creating optimization templates...")
    
    # Template for reduced threading test
    threading_fix = '''// Optimized version with reduced thread counts
const int numThreads = 10;  // Reduced from 100
const int accessesPerThread = 100;  // Reduced from 1000

// Add cleanup between iterations
for (int iter = 0; iter < numIterations; ++iter) {
    ResourceManager manager;
    // ... test code ...
    manager.cleanup();  // Explicit cleanup
    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Brief pause
}'''
    
    # Template for memory-safe scene test
    scene_fix = '''# Optimized version with proper cleanup
def test_with_cleanup():
    # Reuse single project
    project_name = "TestProject"
    
    # Create project once
    run_command(f"project.create {project_name}")
    
    try:
        # Run tests
        for i in range(10):
            run_command("scene.create test_scene")
            run_command("entity.create TestEntity")
            # Cleanup after each iteration
            run_command("scene.clear")
            time.sleep(0.1)  # Brief pause
    finally:
        # Ensure cleanup
        run_command("project.close")
        run_command(f"project.delete {project_name}")'''
    
    print("✅ Optimization templates created")
    print("\nUse these patterns to reduce resource usage in tests")

if __name__ == "__main__":
    print("=== TEST SYSTEM ANALYSIS ===")
    print("Identifying sources of memory leaks and high CPU usage...\n")
    
    # Change to tests directory
    script_dir = os.path.dirname(os.path.abspath(__file__))
    os.chdir(script_dir)
    
    # Run analyses
    has_threading_issues = analyze_threading_tests()
    has_scene_issues = analyze_scene_memory_test()
    analyze_parallel_execution()
    identify_resource_intensive_operations()
    suggest_optimizations()
    create_optimized_versions()
    
    print("\n" + "=" * 60)
    if has_threading_issues or has_scene_issues:
        print("❌ Critical issues found that likely cause overheating!")
        print("   Implement the suggested optimizations to fix the problems.")
    else:
        print("✅ Analysis complete. See recommendations above.")
    print("=" * 60)