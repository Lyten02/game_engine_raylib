#!/usr/bin/env python3
"""Test GameLogicManager commands"""

import sys
import os
import subprocess
import json

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from test_utils import run_cli_command, run_cli_batch, build_engine_if_needed, find_executable

def test_game_logic_commands():
    """Test the new GameLogicManager commands"""
    print("=" * 60)
    print("Testing GameLogicManager commands...")
    print("=" * 60)
    
    # Build engine if needed
    build_engine_if_needed()
    
    # Find the executable
    exe = find_executable()
    if not exe:
        print("ERROR: Could not find game executable")
        return False
    
    # Test 1: List game logics (should be empty initially)
    print("\n1. Testing logic.list (empty)...")
    result = run_cli_command(['logic.list'])
    if not result['success']:
        print(f"FAIL: logic.list failed: {result['error']}")
        return False
    
    if "no active game logics" in result['output'].lower():
        print("PASS: logic.list shows no active logics")
    else:
        print(f"WARNING: Unexpected output: {result['output']}")
    
    # Test 2-4: Register and create in batch (registration doesn't persist between calls)
    print("\n2-4. Testing register, create, and list in batch...")
    result = run_cli_batch([
        'logic.register.example',
        'logic.create ExampleGameLogic',
        'logic.list'
    ])
    
    if not result['success']:
        print(f"FAIL: Batch register/create/list failed: {result['error']}")
        return False
    
    # Check batch results
    if 'data' in result and 'results' in result['data']:
        batch_results = result['data']['results']
        if len(batch_results) >= 3:
            # Check register succeeded
            if batch_results[0]['success'] and "registered" in batch_results[0]['output'].lower():
                print("PASS: Example game logic registered")
            else:
                print(f"FAIL: Registration failed: {batch_results[0]}")
                return False
                
            # Check create succeeded
            if batch_results[1]['success'] and "created" in batch_results[1]['output'].lower():
                print("PASS: Game logic instance created")
            else:
                print(f"FAIL: Creation failed: {batch_results[1]}")
                return False
                
            # Check list shows the logic
            if batch_results[2]['success'] and "ExampleGameLogic" in batch_results[2]['output']:
                print("PASS: logic.list shows ExampleGameLogic")
            else:
                print(f"FAIL: List doesn't show logic: {batch_results[2]}")
                return False
    else:
        print(f"FAIL: Unexpected batch result format")
        return False
    
    # Test 5: Clear all logics (in a new batch)
    print("\n5. Testing logic.clear...")
    result = run_cli_batch([
        'logic.register.example',
        'logic.create ExampleGameLogic',
        'logic.clear',
        'logic.list'
    ])
    
    if not result['success']:
        print(f"FAIL: Clear test failed: {result['error']}")
        return False
    
    if 'data' in result and 'results' in result['data']:
        batch_results = result['data']['results']
        if len(batch_results) >= 4:
            # Check clear succeeded
            if batch_results[2]['success'] and "cleared" in batch_results[2]['output'].lower():
                print("PASS: All game logics cleared")
            else:
                print(f"FAIL: Clear failed: {batch_results[2]}")
                return False
                
            # Check list is empty
            if batch_results[3]['success'] and "no active" in batch_results[3]['output'].lower():
                print("PASS: logic.list shows no active logics after clear")
            else:
                print(f"FAIL: List not empty after clear: {batch_results[3]}")
                return False
    
    
    print("\n" + "=" * 60)
    print("All GameLogicManager tests passed!")
    print("=" * 60)
    return True

if __name__ == "__main__":
    success = test_game_logic_commands()
    sys.exit(0 if success else 1)