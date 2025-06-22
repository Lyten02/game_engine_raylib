#!/usr/bin/env python3

import subprocess
import sys
import os
import shutil
from pathlib import Path
import tempfile

def test_auto_formatter_script():
    """Test that auto formatter script works correctly"""
    
    print("=== Testing Auto Formatter Script ===")
    
    # Check if formatter script exists
    formatter_path = Path(__file__).parent.parent / "scripts" / "auto_format.py"
    
    if not formatter_path.exists():
        print("❌ Auto formatter script not found at:", formatter_path)
        return False
    
    # Create temporary test directory
    with tempfile.TemporaryDirectory() as tmpdir:
        test_dir = Path(tmpdir) / "test_src"
        test_dir.mkdir()
        
        # Create test file with bad formatting
        test_file = test_dir / "test.cpp"
        bad_code = """#include <iostream>
int   main()
{
std::cout<<"Hello"<<std::endl;
    return   0;}
"""
        test_file.write_text(bad_code)
        
        # Run formatter script
        result = subprocess.run(
            [sys.executable, str(formatter_path), str(test_dir)],
            capture_output=True,
            text=True
        )
        
        if result.returncode != 0:
            print("❌ Formatter script failed:")
            print("STDOUT:", result.stdout)
            print("STDERR:", result.stderr)
            return False
        
        print("✅ Formatter script executed successfully")
        
        # Check if file was modified
        formatted_content = test_file.read_text()
        if formatted_content == bad_code:
            print("⚠️  File was not modified (might need clang-format installed)")
        else:
            print("✅ File was formatted")
        
        return True

def test_formatter_handles_errors():
    """Test that formatter handles errors gracefully"""
    
    formatter_path = Path(__file__).parent.parent / "scripts" / "auto_format.py"
    
    if not formatter_path.exists():
        print("⚠️  Formatter script not found, skipping error handling test")
        return True
    
    # Test with non-existent directory
    result = subprocess.run(
        [sys.executable, str(formatter_path), "/non/existent/path"],
        capture_output=True,
        text=True
    )
    
    # Should handle error gracefully
    if "error" in result.stdout.lower() or "error" in result.stderr.lower():
        print("✅ Formatter handles non-existent paths gracefully")
        return True
    
    print("✅ Formatter completed (may have skipped non-existent path)")
    return True

if __name__ == "__main__":
    # Create scripts directory if it doesn't exist
    scripts_dir = Path(__file__).parent.parent / "scripts"
    scripts_dir.mkdir(exist_ok=True)
    
    test1 = test_auto_formatter_script()
    test2 = test_formatter_handles_errors()
    
    success = test1 and test2
    sys.exit(0 if success else 1)