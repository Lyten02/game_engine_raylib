#!/usr/bin/env python3
"""Test that entity.create returns ID in data field"""

import subprocess
import json
import sys
import os

def test_entity_create_returns_id():
    """Test entity.create returns structured data"""
    
    # We are already in the build directory when tests are run
    
    # Execute command
    result = subprocess.run(
        ["./game_engine", "--json", "--headless", "--command", "entity.create"],
        capture_output=True,
        text=True
    )
    
    # Check return code
    assert result.returncode == 0, f"Command failed with code {result.returncode}: {result.stderr}"
    
    # Parse JSON
    try:
        response = json.loads(result.stdout)
    except json.JSONDecodeError as e:
        print(f"Failed to parse JSON: {result.stdout}")
        raise
    
    # Verify response structure
    assert response["success"] == True, f"Command not successful: {response}"
    assert response["data"] is not None, "Data field is None"
    assert "id" in response["data"], "ID not in data field"
    assert isinstance(response["data"]["id"], int), "ID is not an integer"
    assert response["data"]["id"] >= 0, "ID is negative"
    
    print(f"✅ entity.create returns ID in data field: {response['data']['id']}")
    return response["data"]["id"]

def test_entity_create_with_position():
    """Test entity.create with position parameters"""
    
    # We are already in the build directory when tests are run
    
    result = subprocess.run(
        ["./game_engine", "--json", "--headless", "--command", "entity.create 10 20 30"],
        capture_output=True,
        text=True
    )
    
    # Check return code
    assert result.returncode == 0, f"Command failed with code {result.returncode}: {result.stderr}"
    
    # Parse JSON
    response = json.loads(result.stdout)
    
    # Verify response
    assert response["success"] == True, f"Command not successful: {response}"
    assert response["data"] is not None, "Data field is None"
    assert "id" in response["data"], "ID not in data field"
    
    print(f"✅ entity.create with position works, ID: {response['data']['id']}")

def test_entity_create_output_format():
    """Test that entity.create output format is correct"""
    
    # We are already in the build directory when tests are run
    
    result = subprocess.run(
        ["./game_engine", "--json", "--headless", "--command", "entity.create"],
        capture_output=True,
        text=True
    )
    
    response = json.loads(result.stdout)
    
    # Verify the output message contains the entity ID
    assert "Created entity #" in response["output"], f"Output doesn't contain expected message: {response['output']}"
    assert "with Transform component" in response["output"], f"Output doesn't mention Transform component: {response['output']}"
    
    # Extract ID from output and verify it matches data.id
    import re
    match = re.search(r'Created entity #(\d+)', response["output"])
    assert match is not None, f"Could not extract ID from output: {response['output']}"
    output_id = int(match.group(1))
    assert output_id == response["data"]["id"], f"Output ID {output_id} doesn't match data ID {response['data']['id']}"
    
    print(f"✅ entity.create output format is correct and consistent")

if __name__ == "__main__":
    print("Testing entity.create command data field...")
    test_entity_create_returns_id()
    test_entity_create_with_position()
    test_entity_create_output_format()
    print("\n✅ All entity.create tests passed!")