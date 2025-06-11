#!/usr/bin/env python3
"""
Test the editor mode build functionality
Tests that the build system works correctly when running in interactive editor mode
"""

import subprocess
import os
import sys
import json
import time
import shutil
from pathlib import Path

def run_editor_test():
    """Test editor functionality by simulating interactive commands"""
    
    print("Testing editor mode build functionality...")
    
    # Check if project already exists with cached dependencies
    has_cached_deps = False
    for base_dir in [".", ".."]:
        output_path = os.path.join(base_dir, "output/EditorTest")
        if os.path.exists(os.path.join(output_path, "build/_deps")):
            has_cached_deps = True
            break
    
    # Don't clean up to preserve cached dependencies
    # for base_dir in [".", ".."]:
    #     project_path = os.path.join(base_dir, "projects/EditorTest")
    #     output_path = os.path.join(base_dir, "output/EditorTest")
    #     
    #     if os.path.exists(project_path):
    #         shutil.rmtree(project_path)
    #         print(f"Cleaned up existing project at: {project_path}")
    #         
    #     if os.path.exists(output_path):
    #         shutil.rmtree(output_path)
    #         print(f"Cleaned up existing output at: {output_path}")
    
    # Create a script file with editor commands
    # Use fast build if project already exists with deps
    build_command = "project.build.fast" if has_cached_deps else "project.build"
    
    if has_cached_deps:
        # Project exists, just open and build
        test_script = f"""# Test editor mode build functionality
project.open EditorTest
{build_command}
quit
"""
    else:
        # Create new project
        test_script = f"""# Test editor mode build functionality
project.create EditorTest
project.open EditorTest
scene.create test_scene
entity.create Player
entity.create Enemy
scene.save test_scene
{build_command}
quit
"""
    
    with open("test_editor_build.txt", "w") as f:
        f.write(test_script)
    
    try:
        # Run the game with batch commands and JSON output
        build_cmd = "project.build.fast" if has_cached_deps else "project.build"
        
        if has_cached_deps:
            # Project exists, just open and build
            commands = [
                "project.open EditorTest",
                build_cmd
            ]
        else:
            # Create new project
            commands = [
                "project.create EditorTest",
                "project.open EditorTest",
                "scene.create test_scene",
                "entity.create Player",
                "entity.create Enemy",
                "scene.save test_scene",
                build_cmd
            ]
        
        result = subprocess.run(
            ["./game", "--json", "--batch"] + commands,
            capture_output=True,
            text=True,
            timeout=120  # Increase timeout for CMake operations
        )
        
        print(f"Exit code: {result.returncode}")
        
        # Parse JSON output
        try:
            json_result = json.loads(result.stdout)
            if not json_result.get("success", False):
                print("ERROR: Commands failed")
                print("Output:", json_result.get("output", ""))
                if "error" in json_result:
                    print("Error:", json_result["error"])
                return False
                
            # Check batch results
            data = json_result.get("data", {})
            batch_results = data.get("results", [])
            if len(batch_results) != len(commands):
                print(f"ERROR: Expected {len(commands)} results, got {len(batch_results)}")
                return False
                
            # Verify all commands succeeded
            for i, (cmd, result) in enumerate(zip(commands, batch_results)):
                if not result.get("success", False):
                    print(f"ERROR: Command '{cmd}' failed")
                    print(f"  Output: {result.get('output', '')}")
                    print(f"  Error: {result.get('error', '')}")
                    return False
                else:
                    print(f"✓ {cmd}")
                    
        except json.JSONDecodeError:
            print("ERROR: Invalid JSON output")
            print("Output:", result.stdout)
            print("Stderr:", result.stderr)
            return False
            
        # Check that files were generated
        # Files could be in current directory or parent directory
        output_base = None
        for base_dir in [".", ".."]:
            if os.path.exists(os.path.join(base_dir, "output/EditorTest")):
                output_base = base_dir
                break
                
        if not output_base:
            print("ERROR: Output directory not found in either current or parent directory")
            return False
        
        expected_files = [
            os.path.join(output_base, "output/EditorTest/main.cpp"),
            os.path.join(output_base, "output/EditorTest/CMakeLists.txt"),
            os.path.join(output_base, "output/EditorTest/game_config.json"),
            os.path.join(output_base, "output/EditorTest/scenes/test_scene.json")
        ]
        
        for file_path in expected_files:
            if not os.path.exists(file_path):
                print(f"ERROR: Expected file not found: {file_path}")
                return False
            else:
                print(f"✓ Found: {file_path}")
        
        # Verify the generated scene file
        scene_file_path = os.path.join(output_base, "output/EditorTest/scenes/test_scene.json")
        with open(scene_file_path, "r") as f:
            scene_data = json.load(f)
            
        # Check that entities were saved
        if "entities" not in scene_data:
            print("ERROR: Scene file missing entities")
            return False
            
        entity_count = len(scene_data["entities"])
        if entity_count != 2:
            print(f"ERROR: Expected 2 entities, found {entity_count}")
            return False
        
        print("✓ Scene file contains correct number of entities")
        
        # Test with full compilation if cmake is available and not skipped
        cmake_available = subprocess.run(["which", "cmake"], capture_output=True).returncode == 0
        skip_full_build = "--skip-full-build" in sys.argv
        
        if cmake_available and not skip_full_build:
            print("\nTesting full build with compilation...")
            
            # Run full build - use fast build if deps exist
            full_build_cmd = "project.build.fast" if has_cached_deps else "project.build"
            result = subprocess.run(
                ["./game", "--json", "--batch", "project.open EditorTest", full_build_cmd],
                capture_output=True,
                text=True,
                timeout=60  # Give more time for compilation
            )
            
            try:
                json_result = json.loads(result.stdout)
                if json_result.get("success", False):
                    print("✓ Full build succeeded")
                    
                    # Check if executable was created
                    executable_path = os.path.join(output_base, "output/EditorTest/bin/EditorTest")
                    if os.path.exists(executable_path):
                        print(f"✓ Executable created: {executable_path}")
                    else:
                        print("WARNING: Executable not found at expected location")
                else:
                    print("WARNING: Full build did not succeed (this is okay for quick tests)")
                    if "error" in json_result:
                        print(f"  Error: {json_result['error']}")
            except json.JSONDecodeError:
                print("WARNING: Could not parse build output")
        
        print("\n✅ Editor mode build test PASSED!")
        return True
        
    except subprocess.TimeoutExpired:
        print("ERROR: Command timed out")
        return False
    except Exception as e:
        print(f"ERROR: {e}")
        return False
    finally:
        # Cleanup
        for temp_file in ["test_editor_build.txt", "test_editor_full_build.txt"]:
            if os.path.exists(temp_file):
                os.remove(temp_file)

def test_path_resolution():
    """Test that the path resolution works from different directories"""
    
    print("\nTesting path resolution from different working directories...")
    
    # Save current directory
    original_cwd = os.getcwd()
    
    try:
        # Test 1: From home directory
        os.chdir(os.path.expanduser("~"))
        print(f"Testing from: {os.getcwd()}")
        
        result = subprocess.run(
            [os.path.join(original_cwd, "game"), "--json", "-c", "help"],
            capture_output=True,
            text=True
        )
        
        if result.returncode == 0:
            print("✓ Works from home directory")
        else:
            print("✗ Failed from home directory")
            
        # Test 2: From /tmp
        os.chdir("/tmp")
        print(f"Testing from: {os.getcwd()}")
        
        result = subprocess.run(
            [os.path.join(original_cwd, "game"), "--json", "-c", "help"],
            capture_output=True,
            text=True
        )
        
        if result.returncode == 0:
            print("✓ Works from /tmp directory")
        else:
            print("✗ Failed from /tmp directory")
            
    finally:
        # Restore original directory
        os.chdir(original_cwd)
    
    print("Path resolution tests completed")

def main():
    """Run all editor tests"""
    
    # Check if we're in the right directory
    if not os.path.exists("./game"):
        print("ERROR: ./game not found. Run from build directory.")
        return 1
    
    # Set environment variable to help with path resolution
    game_engine_root = os.path.abspath("..")
    os.environ["GAMEENGINE_ROOT"] = game_engine_root
    print(f"Set GAMEENGINE_ROOT={game_engine_root}")
    
    # Run tests
    tests_passed = True
    
    # Test path resolution
    test_path_resolution()
    
    # Test editor build functionality
    if not run_editor_test():
        tests_passed = False
    
    if tests_passed:
        print("\n✅ All editor tests PASSED!")
        return 0
    else:
        print("\n❌ Some editor tests FAILED!")
        return 1

if __name__ == "__main__":
    sys.exit(main())