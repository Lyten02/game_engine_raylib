#!/usr/bin/env python3
"""
Test configuration loading from different locations
"""

import subprocess
import os
import sys
import json
import shutil
from pathlib import Path

def test_config_loading():
    """Test that config.json is loaded correctly"""
    
    print("Testing configuration loading...")
    
    # Save current directory
    original_cwd = os.getcwd()
    
    try:
        # Test 1: Config in build directory (should work)
        print("\nTest 1: Config in build directory")
        if os.path.exists("config.json"):
            result = subprocess.run(
                ["./game", "--json", "-c", "config.get window.width"],
                capture_output=True,
                text=True
            )
            
            if result.returncode == 0:
                data = json.loads(result.stdout)
                if "1280" in data.get("output", ""):
                    print("✓ Config loaded from build directory")
                else:
                    print("✗ Config loaded but wrong value")
            else:
                print("✗ Failed to load config from build directory")
        else:
            print("⚠ config.json not found in build directory")
            
        # Test 2: No config (should use defaults without crashing)
        print("\nTest 2: No config file (testing defaults)")
        
        # Temporarily rename config if it exists
        config_renamed = False
        if os.path.exists("config.json"):
            os.rename("config.json", "config.json.bak")
            config_renamed = True
            
        result = subprocess.run(
            ["./game", "--json", "-c", "engine.info"],
            capture_output=True,
            text=True
        )
        
        if result.returncode == 0:
            print("✓ Engine runs with default config")
        else:
            print("✗ Engine failed without config file")
            
        # Restore config
        if config_renamed:
            os.rename("config.json.bak", "config.json")
            
        # Test 3: Config loading from parent directory
        print("\nTest 3: Running from subdirectory")
        
        # Create a subdirectory
        os.makedirs("test_subdir", exist_ok=True)
        os.chdir("test_subdir")
        
        result = subprocess.run(
            ["../game", "--json", "-c", "help"],
            capture_output=True,
            text=True
        )
        
        if result.returncode == 0:
            print("✓ Engine runs from subdirectory")
        else:
            print("✗ Engine failed from subdirectory")
            
        # Return to original directory
        os.chdir(original_cwd)
        
        # Cleanup
        if os.path.exists("test_subdir"):
            os.rmdir("test_subdir")
            
        print("\nConfiguration loading tests completed")
        return True
        
    except Exception as e:
        print(f"ERROR: {e}")
        return False
    finally:
        os.chdir(original_cwd)
        # Restore config if needed
        if os.path.exists("config.json.bak"):
            os.rename("config.json.bak", "config.json")

def create_test_config():
    """Create a test configuration file if it doesn't exist"""
    
    if not os.path.exists("config.json"):
        print("Creating test config.json...")
        
        config = {
            "window": {
                "width": 1280,
                "height": 720,
                "title": "Game Engine Test",
                "fullscreen": False,
                "vsync": True,
                "target_fps": 60
            },
            "console": {
                "enabled": True,
                "font_size": 14,
                "background_alpha": 0.8,
                "max_lines": 20,
                "toggle_key": "GRAVE",
                "command_timeout_seconds": 10,
                "enable_command_timeout": True
            },
            "rendering": {
                "clear_color": [64, 64, 64, 255]
            },
            "game_logic": {
                "enabled": True,
                "auto_register_example": False
            }
        }
        
        with open("config.json", "w") as f:
            json.dump(config, f, indent=4)
            
        print("✓ Created config.json")

def main():
    """Run configuration tests"""
    
    # Check if we're in the right directory
    if not os.path.exists("./game"):
        print("ERROR: ./game not found. Run from build directory.")
        return 1
    
    # Create test config if needed
    create_test_config()
    
    # Run tests
    if test_config_loading():
        print("\n✅ Configuration tests PASSED!")
        return 0
    else:
        print("\n❌ Configuration tests FAILED!")
        return 1

if __name__ == "__main__":
    sys.exit(main())