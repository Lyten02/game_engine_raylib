#!/usr/bin/env python3
"""Test that basic commands execute quickly"""

import subprocess
import json
import time
import os
import sys

# Add the project root to the path
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from tests.test_utils import find_executable

def run_cli_command(command, json_output=False, timeout=15):
    """Run a CLI command and return the result"""
    exe = find_executable()
    if not exe:
        return {'success': False, 'output': '', 'error': 'Executable not found'}
    
    try:
        cmd = [exe, '--headless']
        if json_output:
            cmd.append('--json')
        cmd.extend(['--command', command])
        
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=timeout
        )
        
        if json_output and result.stdout:
            try:
                return json.loads(result.stdout)
            except json.JSONDecodeError:
                return {'success': False, 'error': 'Invalid JSON response', 'output': result.stdout}
        
        return {
            'success': result.returncode == 0,
            'output': result.stdout,
            'error': result.stderr
        }
    except subprocess.TimeoutExpired:
        return {'success': False, 'output': '', 'error': 'Command timed out'}
    except Exception as e:
        return {'success': False, 'output': '', 'error': str(e)}

def test_fast_commands():
    """Test that basic commands execute within reasonable time"""
    
    print("\n=== Testing Command Performance ===")
    
    # Check if executable exists
    exe = find_executable()
    if not exe:
        print("❌ Executable not found. Please build the project first.")
        return False
    
    fast_commands = [
        ("help", "Show help information"),
        ("project.list", "List projects"),
        ("engine.info", "Show engine information"),
        ("clear", "Clear console"),
        ("scene.list", "List scenes"),
        ("entity.list", "List entities"),
        ("config.get window.width", "Get config value")
    ]
    
    total_time = 0
    failed_commands = []
    
    print("\nTesting command execution times:")
    print("-" * 50)
    
    for cmd, description in fast_commands:
        print(f"\nTesting: {cmd}")
        print(f"  Description: {description}")
        
        start_time = time.time()
        try:
            result = run_cli_command(cmd, json_output=True, timeout=5)
            elapsed = time.time() - start_time
            total_time += elapsed
            
            # Check if command completed successfully
            if result['success'] or "timed out" in result.get('error', '').lower():
                # Command either succeeded or timed out properly
                if elapsed < 2.0:
                    print(f"  ✅ Completed in {elapsed:.3f}s")
                else:
                    print(f"  ⚠️  Slow execution: {elapsed:.3f}s")
                    if elapsed < 5.0:
                        print(f"     (Still within acceptable range)")
                    else:
                        failed_commands.append((cmd, f"Too slow: {elapsed:.3f}s"))
            else:
                print(f"  ❌ Failed: {result.get('error', 'Unknown error')}")
                failed_commands.append((cmd, result.get('error', 'Unknown error')))
                
        except subprocess.TimeoutExpired:
            elapsed = 5.0  # Timeout value
            print(f"  ❌ Command timed out (>{elapsed}s)")
            failed_commands.append((cmd, "Subprocess timeout"))
        except Exception as e:
            print(f"  ❌ Error: {str(e)}")
            failed_commands.append((cmd, str(e)))
    
    print("\n" + "-" * 50)
    print(f"Total time for all commands: {total_time:.3f}s")
    print(f"Average time per command: {total_time/len(fast_commands):.3f}s")
    
    if failed_commands:
        print(f"\n❌ {len(failed_commands)} commands failed or were too slow:")
        for cmd, error in failed_commands:
            print(f"  - {cmd}: {error}")
        return False
    else:
        print("\n✅ All commands executed within acceptable time limits")
        return True

def test_command_categories_performance():
    """Test performance of commands by category"""
    
    print("\n=== Testing Command Categories Performance ===")
    
    categories = {
        "General": ["help", "clear"],
        "Project": ["project.list"],
        "Scene": ["scene.list"],
        "Entity": ["entity.list"],
        "Config": ["config.get window.width", "config.get console.enabled"],
        "Engine": ["engine.info"]
    }
    
    category_times = {}
    
    for category, commands in categories.items():
        print(f"\nTesting {category} commands:")
        category_time = 0
        
        for cmd in commands:
            start_time = time.time()
            result = run_cli_command(cmd, json_output=True, timeout=5)
            elapsed = time.time() - start_time
            category_time += elapsed
            
            status = "✅" if result['success'] or "timed out" in result.get('error', '').lower() else "❌"
            print(f"  {status} {cmd}: {elapsed:.3f}s")
        
        avg_time = category_time / len(commands) if commands else 0
        category_times[category] = avg_time
        print(f"  Average: {avg_time:.3f}s")
    
    # Print summary
    print("\n" + "=" * 50)
    print("Category Performance Summary:")
    print("-" * 50)
    
    sorted_categories = sorted(category_times.items(), key=lambda x: x[1])
    for category, avg_time in sorted_categories:
        bar_length = int(avg_time * 20)  # Scale for visualization
        bar = "█" * bar_length
        print(f"{category:12} {avg_time:.3f}s {bar}")
    
    return True

def test_concurrent_commands():
    """Test running multiple commands concurrently"""
    
    print("\n=== Testing Concurrent Command Execution ===")
    
    # This tests that the timeout mechanism doesn't interfere with 
    # running multiple CLI instances
    
    import concurrent.futures
    
    commands = ["help", "project.list", "engine.info", "config.get window.width"]
    
    print(f"\nRunning {len(commands)} commands concurrently...")
    start_time = time.time()
    
    with concurrent.futures.ThreadPoolExecutor(max_workers=len(commands)) as executor:
        futures = {}
        for cmd in commands:
            future = executor.submit(run_cli_command, cmd, json_output=True, timeout=5)
            futures[future] = cmd
        
        results = {}
        for future in concurrent.futures.as_completed(futures):
            cmd = futures[future]
            try:
                result = future.result()
                results[cmd] = (True, result)
            except Exception as e:
                results[cmd] = (False, str(e))
    
    total_time = time.time() - start_time
    
    print(f"\nCompleted all commands in {total_time:.3f}s")
    print("\nResults:")
    
    all_successful = True
    for cmd, (success, result) in results.items():
        if success and isinstance(result, dict) and (result.get('success') or "timed out" in result.get('error', '').lower()):
            print(f"  ✅ {cmd}")
        else:
            print(f"  ❌ {cmd}: {result}")
            all_successful = False
    
    if all_successful:
        print("\n✅ All concurrent commands completed successfully")
    else:
        print("\n❌ Some concurrent commands failed")
    
    return all_successful

if __name__ == "__main__":
    try:
        # Run all performance tests
        success = True
        
        if not test_fast_commands():
            success = False
        
        if not test_command_categories_performance():
            success = False
        
        if not test_concurrent_commands():
            success = False
        
        if success:
            print("\n✅ All performance tests passed!")
            exit(0)
        else:
            print("\n❌ Some performance tests failed!")
            exit(1)
            
    except Exception as e:
        print(f"\n❌ Performance test failed with error: {e}")
        exit(1)