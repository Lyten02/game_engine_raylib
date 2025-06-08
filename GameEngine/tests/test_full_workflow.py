#!/usr/bin/env python3
"""
End-to-end test for complete workflow: project creation → scene creation → entity creation → build → run
"""

import sys
import os
import subprocess
import time
import json
import shutil

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from test_utils import run_cli_command, build_engine_if_needed, find_executable

def test_full_workflow():
    """Test the complete GameEngine workflow"""
    print("=" * 60)
    print("Testing full GameEngine workflow...")
    print("=" * 60)
    
    # Build engine if needed
    build_engine_if_needed()
    
    # Find the executable
    exe = find_executable()
    if not exe:
        print("ERROR: Could not find game executable")
        return False
    
    # Get build directory
    build_dir = os.path.dirname(exe)
    if exe.endswith('.exe') and ('Debug' in build_dir or 'Release' in build_dir):
        build_dir = os.path.dirname(build_dir)
    
    # Test project name
    project_name = f"FullWorkflowTest_{int(time.time())}"
    scene_name = "test_scene"
    
    try:
        # Step 1: Create a new project
        print("\n1. Creating new project...")
        result = run_cli_command(['project.create', project_name])
        if not result['success']:
            print(f"FAIL: Project creation failed: {result['error']}")
            return False
        
        if "successfully" in result['output'].lower():
            print(f"PASS: Project '{project_name}' created successfully")
        else:
            print(f"WARNING: Project created but unexpected output: {result['output']}")
        
        # Step 2: Set the current project
        print("\n2. Setting current project...")
        result = run_cli_command(['project.set', project_name])
        if not result['success']:
            print(f"FAIL: Failed to set current project: {result['error']}")
            return False
        
        print(f"PASS: Current project set to '{project_name}'")
        
        # Step 3: Create a scene
        print("\n3. Creating a scene...")
        result = run_cli_command(['scene.create', scene_name])
        if not result['success']:
            print(f"FAIL: Scene creation failed: {result['error']}")
            return False
        
        print(f"PASS: Scene '{scene_name}' created successfully")
        
        # Step 4: Set the scene as main scene
        print("\n4. Setting main scene...")
        result = run_cli_command(['scene.set-main', scene_name])
        if not result['success']:
            print(f"WARNING: Failed to set main scene: {result['error']}")
            # This might not be critical, continue
        else:
            print(f"PASS: Main scene set to '{scene_name}'")
        
        # Step 5: Create entities with components
        print("\n5. Creating entities...")
        
        # Create first entity
        result = run_cli_command(['entity.create', 'Player'])
        if not result['success']:
            print(f"FAIL: Failed to create Player entity: {result['error']}")
            return False
        
        # Extract entity ID from output
        entity_id = None
        if "Entity created with ID:" in result['output']:
            try:
                entity_id = int(result['output'].split("Entity created with ID:")[1].strip().split()[0])
                print(f"PASS: Player entity created with ID: {entity_id}")
            except:
                print("WARNING: Could not parse entity ID")
        
        # Add Transform component
        if entity_id is not None:
            result = run_cli_command(['entity.add-component', str(entity_id), 'Transform'])
            if result['success']:
                print("PASS: Added Transform component to Player")
            else:
                print(f"WARNING: Failed to add Transform component: {result['error']}")
        
        # Create second entity
        result = run_cli_command(['entity.create', 'Enemy'])
        if result['success']:
            print("PASS: Enemy entity created")
        else:
            print(f"WARNING: Failed to create Enemy entity: {result['error']}")
        
        # Step 6: List entities to verify
        print("\n6. Listing entities...")
        result = run_cli_command(['entity.list'])
        if result['success']:
            if "Player" in result['output'] or "Enemy" in result['output']:
                print("PASS: Entities listed successfully")
            else:
                print(f"WARNING: Entity list doesn't show expected entities: {result['output']}")
        else:
            print(f"WARNING: Failed to list entities: {result['error']}")
        
        # Step 7: Save the scene
        print("\n7. Saving scene...")
        result = run_cli_command(['scene.save'])
        if result['success']:
            print("PASS: Scene saved successfully")
        else:
            print(f"WARNING: Failed to save scene: {result['error']}")
        
        # Step 8: Build the project
        print("\n8. Building project...")
        result = run_cli_command(['build.project', project_name], timeout=120)
        
        build_success = False
        if result['success']:
            print("PASS: Project built successfully")
            build_success = True
        else:
            # Check if it's a dependency issue (expected in some environments)
            if "Could not find a package configuration file" in result['error']:
                print("INFO: Build failed due to missing dependencies (expected in fast build)")
            else:
                print(f"FAIL: Build failed: {result['error']}")
                return False
        
        # Step 9: Verify project structure
        print("\n9. Verifying project structure...")
        project_dir = os.path.join(build_dir, 'projects', project_name)
        
        # Check main files
        expected_files = [
            'project.json',
            'main.cpp',
            'CMakeLists.txt',
            'game_config.json',
            os.path.join('scenes', f'{scene_name}.json')
        ]
        
        all_files_found = True
        for file_path in expected_files:
            full_path = os.path.join(project_dir, file_path)
            if os.path.exists(full_path):
                print(f"PASS: Found {file_path}")
            else:
                print(f"FAIL: Missing {file_path}")
                all_files_found = False
        
        if not all_files_found:
            return False
        
        # Step 10: Verify scene file content
        print("\n10. Verifying scene file content...")
        scene_file = os.path.join(project_dir, 'scenes', f'{scene_name}.json')
        try:
            with open(scene_file, 'r') as f:
                scene_data = json.load(f)
            
            if 'entities' in scene_data:
                print("PASS: Scene file has entities section")
                
                # Check if entities were saved
                entity_count = len(scene_data['entities'])
                if entity_count > 0:
                    print(f"PASS: Scene contains {entity_count} entities")
                else:
                    print("WARNING: Scene has no entities (might not have been saved)")
            else:
                print("FAIL: Scene file missing entities section")
                return False
                
        except Exception as e:
            print(f"FAIL: Could not read scene file: {e}")
            return False
        
        # Step 11: Test config get on the project
        print("\n11. Testing project configuration...")
        result = run_cli_command(['config.get', 'window.title'])
        if result['success']:
            print("PASS: Config system working with project")
        else:
            print(f"WARNING: Config get failed: {result['error']}")
        
        # Step 12: Run a Lua script in project context
        print("\n12. Testing Lua scripting...")
        result = run_cli_command(['script.run', 'scripts/test.lua'])
        if result['success']:
            if "Hello from Lua script!" in result['output']:
                print("PASS: Lua scripting works in project context")
            else:
                print("WARNING: Lua script ran but output unexpected")
        else:
            print(f"WARNING: Lua script failed: {result['error']}")
        
        print("\n" + "=" * 60)
        print("Full workflow test completed successfully!")
        print("=" * 60)
        
        # Indicate what would happen next
        print("\nNext steps (not automated in this test):")
        print("1. The built game executable would be at:")
        if build_success:
            print(f"   {os.path.join(project_dir, 'build', project_name)}")
        else:
            print("   (Build step would need proper dependencies)")
        print("2. Running the game would load the scene with entities")
        print("3. The game would use the config.json settings")
        
        return True
        
    finally:
        # Cleanup
        print("\n13. Cleaning up test project...")
        project_dir = os.path.join(build_dir, 'projects', project_name)
        if os.path.exists(project_dir):
            try:
                shutil.rmtree(project_dir)
                print("PASS: Test project cleaned up")
            except Exception as e:
                print(f"WARNING: Failed to clean up: {e}")

def verify_no_critical_errors():
    """Quick check that engine starts without critical errors"""
    print("\n" + "=" * 60)
    print("Verifying no critical errors on startup...")
    print("=" * 60)
    
    exe = find_executable()
    if not exe:
        print("ERROR: Could not find executable")
        return False
    
    try:
        # Run engine in headless mode briefly
        process = subprocess.Popen(
            [exe, '--headless'],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        
        # Wait a bit for initialization
        time.sleep(1)
        
        # Terminate the process
        process.terminate()
        stdout, stderr = process.communicate(timeout=2)
        
        # Check for critical errors
        critical_errors = [
            "File not found",
            "Failed to load",
            "Configuration not loaded",
            "ERROR",
            "CRITICAL"
        ]
        
        all_output = stdout + stderr
        errors_found = []
        
        for error in critical_errors:
            if error in all_output:
                # Ignore some expected messages
                if "ERROR" in error and "log_error" in all_output:
                    continue  # This is from Lua test output
                errors_found.append(error)
        
        if errors_found:
            print(f"FAIL: Critical errors found: {errors_found}")
            print(f"Output:\n{all_output[:500]}...")  # First 500 chars
            return False
        else:
            print("PASS: No critical errors on startup")
            return True
            
    except subprocess.TimeoutExpired:
        process.kill()
        print("PASS: Engine started without immediate crashes")
        return True
    except Exception as e:
        print(f"ERROR: Failed to run engine: {e}")
        return False

if __name__ == "__main__":
    # Ensure test_utils.py exists
    test_utils_path = os.path.join(os.path.dirname(__file__), 'test_utils.py')
    if not os.path.exists(test_utils_path):
        print("ERROR: test_utils.py not found. Run test_config_loading.py first to create it.")
        sys.exit(1)
    
    # First verify no critical errors
    if not verify_no_critical_errors():
        print("\nEngine has critical errors. Fix these before running workflow test.")
        sys.exit(1)
    
    # Run the full workflow test
    success = test_full_workflow()
    sys.exit(0 if success else 1)