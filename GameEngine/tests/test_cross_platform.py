#!/usr/bin/env python3

import sys
import os
import platform
import subprocess
import shutil
from pathlib import Path

def test_platform_specific_build():
    """Test platform-specific build configurations"""
    
    print(f"=== Cross-Platform Build Test ===")
    print(f"Platform: {platform.system()}")
    print(f"Python: {sys.version}")
    print(f"Architecture: {platform.machine()}")
    
    # Check for required tools
    tools = {
        'cmake': 'CMake',
        'make': 'Make' if platform.system() != 'Windows' else None,
        'mingw32-make': 'MinGW Make' if platform.system() == 'Windows' else None,
    }
    
    missing_tools = []
    for tool, name in tools.items():
        if name and not shutil.which(tool):
            missing_tools.append(name)
    
    if missing_tools:
        print(f"⚠️  Missing tools: {', '.join(missing_tools)}")
        return False
    
    # Platform-specific checks
    if platform.system() == 'Linux':
        print("\n🐧 Linux-specific checks:")
        # Check for X11 libraries
        libs_to_check = ['libGL.so', 'libX11.so']
        for lib in libs_to_check:
            result = subprocess.run(['ldconfig', '-p'], capture_output=True, text=True)
            if lib in result.stdout:
                print(f"  ✅ {lib} found")
            else:
                print(f"  ⚠️  {lib} might be missing")
    
    elif platform.system() == 'Darwin':
        print("\n🍎 macOS-specific checks:")
        # Check for frameworks
        frameworks = ['/System/Library/Frameworks/OpenGL.framework']
        for fw in frameworks:
            if Path(fw).exists():
                print(f"  ✅ {fw} found")
            else:
                print(f"  ❌ {fw} missing")
    
    elif platform.system() == 'Windows':
        print("\n🪟 Windows-specific checks:")
        # Check for MinGW
        gcc_path = shutil.which('gcc')
        if gcc_path:
            print(f"  ✅ GCC found: {gcc_path}")
        else:
            print(f"  ❌ GCC not found")
    
    # Test CMake configuration works
    print("\n📦 Testing CMake configuration...")
    build_dir = Path.cwd()
    
    # Check if CMakeCache.txt exists
    if (build_dir / "CMakeCache.txt").exists():
        print("  ✅ CMake already configured")
        
        # Check key variables
        with open(build_dir / "CMakeCache.txt", 'r') as f:
            content = f.read()
            if 'CMAKE_CXX_COMPILER:' in content:
                compiler_line = [l for l in content.split('\n') if 'CMAKE_CXX_COMPILER:' in l][0]
                print(f"  ℹ️  C++ Compiler: {compiler_line.split('=')[1]}")
    else:
        print("  ⚠️  CMake not yet configured")
    
    # Check for platform-specific build files
    print("\n📄 Checking platform files...")
    platform_config = Path(__file__).parent.parent / "cmake" / "PlatformConfig.cmake"
    if platform_config.exists():
        print(f"  ✅ PlatformConfig.cmake found")
    else:
        print(f"  ❌ PlatformConfig.cmake missing")
    
    # Test that game_engine executable exists
    print("\n🎮 Checking game engine executable...")
    exe_name = "game_engine" + (".exe" if platform.system() == 'Windows' else "")
    exe_path = build_dir / exe_name
    
    if exe_path.exists():
        print(f"  ✅ {exe_name} found")
        # Check if it's executable
        if platform.system() != 'Windows' and os.access(exe_path, os.X_OK):
            print(f"  ✅ {exe_name} is executable")
    else:
        print(f"  ❌ {exe_name} not found")
    
    print("\n✅ Platform validation complete")
    return True

if __name__ == "__main__":
    success = test_platform_specific_build()
    sys.exit(0 if success else 1)