#!/usr/bin/env python3

import subprocess
import sys
import os
from pathlib import Path
import shutil

def test_cmake_configuration():
    """Test that CMake can properly configure the project"""
    build_dir = Path(__file__).parent.parent / "build"
    
    print("=== Testing CMake configuration ===")
    
    # Check if we have CMake
    if not shutil.which('cmake'):
        print("❌ CMake not found!")
        return False
    
    # Check CMake version
    result = subprocess.run(['cmake', '--version'], capture_output=True, text=True)
    print(f"CMake version: {result.stdout.split()[2]}")
    
    # Test configuration
    cmd = ['cmake', '..', '-DCMAKE_BUILD_TYPE=Release']
    
    print(f"Running: {' '.join(cmd)}")
    result = subprocess.run(cmd, cwd=build_dir, capture_output=True, text=True)
    
    if result.returncode != 0:
        print("❌ CMake configuration failed!")
        print("STDOUT:", result.stdout)
        print("STDERR:", result.stderr)
        
        # Common issues
        if "Could not find a package configuration file" in result.stderr:
            print("\n⚠️  Missing package configuration files")
        if "CMake Error at" in result.stderr:
            print("\n⚠️  CMake error detected")
        if "No CMAKE_CXX_COMPILER could be found" in result.stderr:
            print("\n⚠️  C++ compiler not found")
        
        return False
    
    print("✅ CMake configuration successful")
    return True

def test_compiler_availability():
    """Test that required compilers are available"""
    print("\n=== Testing compiler availability ===")
    
    compilers = {
        'g++': ['g++', '--version'],
        'clang++': ['clang++', '--version'],
        'gcc': ['gcc', '--version'],
        'clang': ['clang', '--version']
    }
    
    available = []
    for name, cmd in compilers.items():
        if shutil.which(cmd[0]):
            result = subprocess.run(cmd, capture_output=True, text=True)
            if result.returncode == 0:
                version = result.stdout.split('\n')[0]
                print(f"✅ {name}: {version}")
                available.append(name)
        else:
            print(f"❌ {name}: not found")
    
    if not available:
        print("\n❌ No C++ compiler found!")
        return False
    
    return True

def test_dependencies():
    """Test that required dependencies are available"""
    print("\n=== Testing dependencies ===")
    
    # Check for required system libraries on Linux
    if sys.platform.startswith('linux'):
        libs = [
            'libgl1-mesa-dev',
            'libx11-dev',
            'libxrandr-dev',
            'libxi-dev'
        ]
        
        for lib in libs:
            # Check using pkg-config or ldconfig
            result = subprocess.run(['ldconfig', '-p'], capture_output=True, text=True)
            lib_base = lib.replace('-dev', '').replace('lib', '')
            if lib_base in result.stdout:
                print(f"✅ {lib}: found")
            else:
                print(f"⚠️  {lib}: might be missing")
    
    return True

def test_build_environment():
    """Test the build environment setup"""
    print("\n=== Testing build environment ===")
    
    # Check environment variables
    important_vars = ['CC', 'CXX', 'CMAKE_PREFIX_PATH', 'PKG_CONFIG_PATH']
    
    for var in important_vars:
        value = os.environ.get(var, 'not set')
        print(f"{var}: {value}")
    
    # Check if we're in CI
    if os.environ.get('CI', '').lower() == 'true':
        print("\n✅ Running in CI environment")
        
        # GitHub Actions specific
        if os.environ.get('GITHUB_ACTIONS') == 'true':
            print("Platform: GitHub Actions")
            print(f"Runner OS: {os.environ.get('RUNNER_OS', 'unknown')}")
            print(f"Runner Arch: {os.environ.get('RUNNER_ARCH', 'unknown')}")
    
    return True

def main():
    """Run all diagnostic tests"""
    print("🔍 Game Engine Build Diagnostics")
    print("=" * 50)
    
    tests = [
        test_compiler_availability,
        test_build_environment,
        test_dependencies,
        test_cmake_configuration
    ]
    
    failed = False
    for test in tests:
        if not test():
            failed = True
    
    print("\n" + "=" * 50)
    
    if failed:
        print("❌ Build diagnostics found issues")
        return 1
    else:
        print("✅ Build environment looks good")
        return 0

if __name__ == "__main__":
    sys.exit(main())