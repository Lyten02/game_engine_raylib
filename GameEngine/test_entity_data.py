#!/usr/bin/env python3
"""Quick test to verify entity.create returns data field with ID"""

import subprocess
import json
import os

os.chdir('build')

# Test 1: entity.create without parameters
print("=== Test 1: entity.create without parameters ===")
result = subprocess.run(
    ["./game_engine", "--json", "--headless", "--command", "entity.create"],
    capture_output=True,
    text=True
)

print("Return code:", result.returncode)

try:
    response = json.loads(result.stdout)
    print("Parsed JSON:")
    print(json.dumps(response, indent=2))
    
    if response.get("data") and "id" in response["data"]:
        print(f"✅ SUCCESS: entity.create returns ID in data field: {response['data']['id']}")
    else:
        print("❌ FAIL: No ID in data field")
        
except Exception as e:
    print(f"Error parsing JSON: {e}")

# Test 2: entity.create with position parameters
print("\n=== Test 2: entity.create with position parameters ===")
result = subprocess.run(
    ["./game_engine", "--json", "--headless", "--command", "entity.create 100 200 300"],
    capture_output=True,
    text=True
)

print("Return code:", result.returncode)

try:
    response = json.loads(result.stdout)
    print("Parsed JSON:")
    print(json.dumps(response, indent=2))
    
    if response.get("data") and "id" in response["data"]:
        print(f"✅ SUCCESS: entity.create with position returns ID in data field: {response['data']['id']}")
    else:
        print("❌ FAIL: No ID in data field")
        
except Exception as e:
    print(f"Error parsing JSON: {e}")