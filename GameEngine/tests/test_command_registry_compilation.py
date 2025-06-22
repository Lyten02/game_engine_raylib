#!/usr/bin/env python3
"""Test command registry compilation fixes"""

import subprocess
import os
import sys

def test_command_registry_project_compiles():
    """Test that command_registry_project.cpp compiles without errors"""
    print("Testing command_registry_project.cpp compilation...")
    
    # Check if file exists
    if not os.path.exists("src/engine/command_registry_project.cpp"):
        print("❌ command_registry_project.cpp not found")
        return False
    
    # Try to compile just this file (syntax check)
    compile_cmd = [
        "clang++", "-std=c++20", "-fsyntax-only",
        "-I", "src",
        "-I", "build/_deps/nlohmann_json-src/include",
        "-I", "build/_deps/spdlog-src/include",
        "-I", "build/_deps/entt-src/src",
        "src/engine/command_registry_project.cpp"
    ]
    
    result = subprocess.run(compile_cmd, capture_output=True, text=True)
    
    if result.returncode == 0:
        print("✅ command_registry_project.cpp compiles successfully")
        return True
    else:
        print("❌ Compilation errors:")
        print(result.stderr)
        return False

def test_command_registry_engine_compiles():
    """Test that command_registry_engine.cpp compiles without errors"""
    print("\nTesting command_registry_engine.cpp compilation...")
    
    # Check if file exists
    if not os.path.exists("src/engine/command_registry_engine.cpp"):
        print("❌ command_registry_engine.cpp not found")
        return False
    
    # Try to compile just this file (syntax check)
    compile_cmd = [
        "clang++", "-std=c++20", "-fsyntax-only",
        "-I", "src",
        "-I", "build/_deps/nlohmann_json-src/include",
        "-I", "build/_deps/spdlog-src/include",
        "-I", "build/_deps/entt-src/src",
        "src/engine/command_registry_engine.cpp"
    ]
    
    result = subprocess.run(compile_cmd, capture_output=True, text=True)
    
    if result.returncode == 0:
        print("✅ command_registry_engine.cpp compiles successfully")
        return True
    else:
        print("❌ Compilation errors:")
        print(result.stderr)
        return False

def test_lambda_captures():
    """Test that lambda captures are correct"""
    print("\nTesting lambda capture patterns...")
    
    # Check command_registry_project.cpp for problematic captures
    with open("src/engine/command_registry_project.cpp", "r") as f:
        content = f.read()
        
        # Check for lambda with this capture in wrong context
        if "[this]()" in content and "processor->" not in content[:content.find("[this]()")]:
            print("❌ Lambda with [this] capture outside of class method context")
            return False
    
    print("✅ Lambda captures look correct")
    return True

def test_processor_scope():
    """Test that processor variable is properly scoped"""
    print("\nTesting processor variable scope...")
    
    files = [
        "src/engine/command_registry_project.cpp",
        "src/engine/command_registry_engine.cpp"
    ]
    
    for file in files:
        with open(file, "r") as f:
            content = f.read()
            
            # Simple check for processor usage without declaration
            lines = content.split('\n')
            for i, line in enumerate(lines):
                if "processor->" in line:
                    # Check if we're inside a function that has processor parameter
                    # This is a simplified check
                    found_param = False
                    for j in range(max(0, i-50), i):
                        if "CommandProcessor* processor" in lines[j]:
                            found_param = True
                            break
                    
                    if not found_param:
                        print(f"❌ processor used without proper declaration in {file} line {i+1}")
                        return False
    
    print("✅ processor scope looks correct")
    return True

if __name__ == "__main__":
    print("=== Command Registry Compilation Tests ===")
    
    # These tests should fail initially
    tests = [
        test_command_registry_project_compiles,
        test_command_registry_engine_compiles,
        test_lambda_captures,
        test_processor_scope
    ]
    
    all_passed = True
    for test in tests:
        if not test():
            all_passed = False
    
    if not all_passed:
        print("\n❌ Some tests failed (expected in Red phase)")
        sys.exit(1)
    else:
        print("\n✅ All tests passed")
        sys.exit(0)