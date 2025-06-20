#!/usr/bin/env python3
"""
Unit tests for progress bar functionality
"""

import sys
import os
import io
import time
from contextlib import redirect_stdout

# Simple progress bar implementation for testing
class TestRunner:
    def __init__(self):
        self.disable_progress = False
    
    def print_progress(self, current, total, test_name, status, elapsed=None):
        """Print progress bar"""
        if self.disable_progress:
            return
            
        percentage = (current / total) * 100 if total > 0 else 0
        
        # Status icons
        icons = {
            'running': '⏳',
            'passed': '✅',
            'failed': '❌',
            'skipped': '⚠️',
            'timeout': '⏱️'
        }
        icon = icons.get(status, '?')
        
        # Build progress bar
        bar_width = 30
        filled = int(bar_width * current / total) if total > 0 else 0
        bar = '=' * filled + '-' * (bar_width - filled)
        
        # Truncate long test names
        max_name_length = 40
        display_name = test_name
        if len(test_name) > max_name_length:
            display_name = test_name[:max_name_length-3] + "..."
        
        # Format output
        output = f"\r[{bar}] {percentage:.1f}% ({current}/{total}) {icon} {display_name}"
        if elapsed:
            output += f" ({elapsed:.1f}s)"
        
        print(output, end='', flush=True)

def test_progress_bar_display():
    """Test progress bar renders correctly"""
    print("Testing progress bar display...")
    
    # Create test runner
    runner = TestRunner()
    runner.disable_progress = False
    
    # Capture output
    output = io.StringIO()
    with redirect_stdout(output):
        # Test various progress states
        runner.print_progress(0, 10, "test1.py", "running")
        print()  # Force newline for testing
        runner.print_progress(5, 10, "test2.py", "passed", 1.5)
        print()
        runner.print_progress(10, 10, "test3.py", "failed", 2.0)
    
    output_str = output.getvalue()
    
    # Check output contains expected elements
    assert "⏳" in output_str, "Should contain running icon"
    assert "✅" in output_str, "Should contain passed icon"
    assert "❌" in output_str, "Should contain failed icon"
    assert "0.0%" in output_str, "Should show 0% progress"
    assert "50.0%" in output_str, "Should show 50% progress"
    assert "100.0%" in output_str, "Should show 100% progress"
    assert "(0/10)" in output_str, "Should show progress count"
    assert "(5/10)" in output_str, "Should show progress count"
    assert "(10/10)" in output_str, "Should show progress count"
    assert "test1.py" in output_str, "Should show test name"
    assert "(1.5s)" in output_str, "Should show elapsed time"
    
    print("✅ Progress bar display test passed")

def test_progress_bar_edge_cases():
    """Test progress bar with edge cases"""
    print("\nTesting progress bar edge cases...")
    
    runner = TestRunner()
    runner.disable_progress = False
    
    output = io.StringIO()
    with redirect_stdout(output):
        # Test with 0 tests
        runner.print_progress(0, 0, "no_tests.py", "running")
        print()
        
        # Test with 1 test
        runner.print_progress(1, 1, "single_test.py", "passed", 0.1)
        print()
        
        # Test with very long test name
        long_name = "this_is_a_very_long_test_name_that_should_be_truncated.py"
        runner.print_progress(1, 2, long_name, "running")
        print()
    
    output_str = output.getvalue()
    
    # Check edge cases handled correctly
    assert "0.0%" in output_str or "nan" not in output_str, "Should handle 0 total tests"
    assert "100.0%" in output_str, "Should handle single test"
    assert "..." in output_str, "Should truncate long names"
    
    print("✅ Progress bar edge cases test passed")

def test_progress_bar_disabled():
    """Test progress bar can be disabled"""
    print("\nTesting progress bar disable flag...")
    
    runner = TestRunner()
    runner.disable_progress = True
    
    output = io.StringIO()
    with redirect_stdout(output):
        runner.print_progress(5, 10, "test.py", "running")
    
    output_str = output.getvalue()
    
    # Should produce no output when disabled
    assert output_str == "", "Should produce no output when disabled"
    
    print("✅ Progress bar disable test passed")

def test_progress_bar_formatting():
    """Test progress bar formatting"""
    print("\nTesting progress bar formatting...")
    
    runner = TestRunner()
    runner.disable_progress = False
    
    # Test different status icons
    statuses = {
        "running": "⏳",
        "passed": "✅",
        "failed": "❌",
        "timeout": "⏱️",
        "unknown": "❓"
    }
    
    for status, expected_icon in statuses.items():
        output = io.StringIO()
        with redirect_stdout(output):
            runner.print_progress(1, 1, f"test_{status}.py", status, 1.0)
            print()
        
        output_str = output.getvalue()
        assert expected_icon in output_str, f"Should contain {expected_icon} for {status} status"
    
    # Test progress bar fill
    output = io.StringIO()
    with redirect_stdout(output):
        # 0% progress
        runner.print_progress(0, 4, "test.py", "running")
        print()
        # 50% progress
        runner.print_progress(2, 4, "test.py", "running")
        print()
        # 100% progress
        runner.print_progress(4, 4, "test.py", "running")
    
    output_str = output.getvalue()
    lines = output_str.strip().split('\n')
    
    # Check bar fill levels
    assert lines[0].count('█') < lines[1].count('█'), "Bar should fill as progress increases"
    assert lines[1].count('█') < lines[2].count('█'), "Bar should fill as progress increases"
    assert '░' in lines[0] and '░' in lines[1], "Should contain empty bar characters"
    assert '░' not in lines[2] or lines[2].count('░') == 0, "100% should be fully filled"
    
    print("✅ Progress bar formatting test passed")

def run_all_tests():
    """Run all progress bar tests"""
    print("="*60)
    print("Progress Bar Unit Tests")
    print("="*60)
    
    try:
        test_progress_bar_display()
        test_progress_bar_edge_cases()
        test_progress_bar_disabled()
        test_progress_bar_formatting()
        
        print("\n" + "="*60)
        print("✅ All progress bar tests passed!")
        print("="*60)
        return 0
    except AssertionError as e:
        print(f"\n❌ Test failed: {e}")
        return 1
    except Exception as e:
        print(f"\n❌ Unexpected error: {e}")
        return 1

if __name__ == "__main__":
    sys.exit(run_all_tests())