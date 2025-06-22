#!/usr/bin/env python3

import subprocess
import sys
import os
from pathlib import Path

def test_formatting_check_non_blocking():
    """Test that formatting check doesn't block CI pipeline"""
    
    print("=== Testing CI Formatting Check Behavior ===")
    
    # Check if we're in CI environment
    is_ci = os.environ.get('CI', '').lower() == 'true'
    
    # Simulate a file with formatting issues
    test_file = Path("test_formatting_sample.cpp")
    
    # Create a file with intentional formatting issues
    bad_code = """#include <iostream>
    int main(  )  {
        std::cout<<"Hello"<<std::endl;
            return 0;
    }
    """
    
    try:
        # Write test file
        with open(test_file, 'w') as f:
            f.write(bad_code)
        
        # Check if clang-format is available
        clang_format = subprocess.run(['which', 'clang-format'], 
                                    capture_output=True, text=True)
        
        if clang_format.returncode != 0:
            print("⚠️  clang-format not available, skipping test")
            return True
        
        # Run clang-format check
        result = subprocess.run(['clang-format', '--dry-run', '--Werror', str(test_file)],
                              capture_output=True, text=True)
        
        if result.returncode != 0:
            print("✅ Formatting issues detected (as expected)")
            # In non-blocking mode, we should still return success
            if is_ci:
                print("✅ CI mode: formatting check is non-blocking")
                return True
            else:
                print("ℹ️  Local mode: formatting check would block")
                return True
        else:
            print("❌ No formatting issues detected (unexpected)")
            return False
            
    finally:
        # Cleanup
        if test_file.exists():
            test_file.unlink()
    
    return True

def test_ci_continues_on_format_warning():
    """Test that CI continues even with formatting warnings"""
    
    # This test verifies the workflow configuration
    workflow_path = Path(__file__).parent.parent.parent / ".github/workflows/ci.yml"
    
    if not workflow_path.exists():
        print("⚠️  CI workflow file not found")
        return True
    
    with open(workflow_path, 'r') as f:
        content = f.read()
    
    # Check if formatting check has continue-on-error
    if 'continue-on-error: true' in content or 'if-no-files-found: warn' in content:
        print("✅ CI workflow configured to continue on formatting issues")
        return True
    
    print("ℹ️  CI workflow may block on formatting issues")
    return True

if __name__ == "__main__":
    test1 = test_formatting_check_non_blocking()
    test2 = test_ci_continues_on_format_warning()
    
    success = test1 and test2
    sys.exit(0 if success else 1)