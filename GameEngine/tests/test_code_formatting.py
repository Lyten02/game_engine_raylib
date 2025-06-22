#!/usr/bin/env python3

import subprocess
import sys
import os
import shutil
from pathlib import Path

def test_clang_format_check():
    """Test that all C++ code follows clang-format rules"""
    
    # Check if clang-format is available
    if not shutil.which('clang-format'):
        print("⚠️  clang-format not found, skipping formatting test")
        print("   Install clang-format to enable formatting checks")
        # Check if we're in CI environment
        if os.environ.get('CI', '').lower() == 'true':
            print("❌ clang-format is required in CI environment")
            return False
        return True  # Don't fail if tool is not available locally
    
    # Find all C++ source files
    src_dir = Path(__file__).parent.parent / "src"
    if not src_dir.exists():
        print(f"❌ Source directory not found: {src_dir}")
        return False
    
    cpp_files = []
    for pattern in ["*.cpp", "*.h", "*.hpp"]:
        cpp_files.extend(src_dir.rglob(pattern))
    
    if not cpp_files:
        print("⚠️  No C++ files found to check")
        return True
    
    print(f"Checking formatting for {len(cpp_files)} files...")
    
    # Run clang-format in dry-run mode
    unformatted_files = []
    for file in cpp_files:
        result = subprocess.run(
            ['clang-format', '--dry-run', '--Werror', str(file)],
            capture_output=True,
            text=True
        )
        
        if result.returncode != 0:
            unformatted_files.append((file, result.stderr))
    
    if unformatted_files:
        print(f"❌ {len(unformatted_files)} files need formatting:")
        for file, error in unformatted_files[:5]:  # Show first 5 with errors
            print(f"   - {file.relative_to(src_dir.parent)}")
            if error:
                # Show first line of error
                first_error = error.strip().split('\n')[0]
                print(f"     {first_error}")
        if len(unformatted_files) > 5:
            print(f"   ... and {len(unformatted_files) - 5} more")
        
        print("\nTo fix formatting, run:")
        print("  cd GameEngine && find src -name '*.cpp' -o -name '*.h' -o -name '*.hpp' | xargs clang-format -i")
        return False
    
    print("✅ All files are properly formatted")
    return True

if __name__ == "__main__":
    success = test_clang_format_check()
    sys.exit(0 if success else 1)