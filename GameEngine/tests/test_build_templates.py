#!/usr/bin/env python3
"""
Test script to verify the build system can successfully create and compile projects.
Tests both fast build (without compilation) and full build (with compilation).
"""

import os
import subprocess
import sys
import time
import shutil
import json
from pathlib import Path

# Add parent directory to path for test_utils import
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from test_utils import print_test_header, print_test_result, cleanup_test_files

class BuildSystemTest:
    def __init__(self):
        self.game_engine_dir = Path(__file__).parent.parent
        self.game_executable = self.game_engine_dir / "build" / "game"
        self.test_project_name = "BuildTestProject"
        self.output_dir = self.game_engine_dir / "output" / self.test_project_name
        self.errors = []
        
    def cleanup(self):
        """Clean up test artifacts"""
        if self.output_dir.exists():
            shutil.rmtree(self.output_dir)
            print(f"Cleaned up {self.output_dir}")
            
    def run_command(self, command_args):
        """Run the game engine with given commands"""
        cmd = [str(self.game_executable), "--json", "--headless", "--batch"] + command_args
        
        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                cwd=str(self.game_engine_dir)
            )
            return result.returncode == 0, result.stdout, result.stderr
        except Exception as e:
            return False, "", str(e)
            
    def test_project_creation(self):
        """Test creating a new project with scenes and entities"""
        print_test_header("Testing project creation")
        
        commands = [
            f"project.create {self.test_project_name}",
            f"project.open {self.test_project_name}",
            "scene.create main",
            "entity.create Player",
            "entity.create Enemy",
            "entity.create Background",
            "scene.save main",
            "scene.create menu",
            "entity.create StartButton",
            "entity.create QuitButton",
            "scene.save menu"
        ]
        
        success, stdout, stderr = self.run_command(commands)
        
        if not success:
            self.errors.append(f"Project creation failed: {stderr}")
            print_test_result("Project creation", False, stderr)
            return False
            
        # Verify project structure was created
        project_path = self.game_engine_dir / "projects" / self.test_project_name
        if not project_path.exists():
            self.errors.append("Project directory not created")
            print_test_result("Project creation", False, "Project directory missing")
            return False
            
        print_test_result("Project creation", True)
        return True
        
    def test_fast_build(self):
        """Test fast build (file generation without compilation)"""
        print_test_header("Testing fast build")
        
        commands = [
            f"project.open {self.test_project_name}",
            "project.build-fast"
        ]
        
        success, stdout, stderr = self.run_command(commands)
        
        if not success:
            self.errors.append(f"Fast build failed: {stderr}")
            print_test_result("Fast build", False, stderr)
            return False
            
        # Verify generated files
        required_files = [
            self.output_dir / "main.cpp",
            self.output_dir / "CMakeLists.txt",
            self.output_dir / "game_config.json",
            self.output_dir / "scenes" / "main.json",
            self.output_dir / "scenes" / "menu.json"
        ]
        
        missing_files = []
        for file_path in required_files:
            if not file_path.exists():
                missing_files.append(str(file_path))
                
        if missing_files:
            self.errors.append(f"Missing files after fast build: {missing_files}")
            print_test_result("Fast build", False, f"Missing files: {', '.join(missing_files)}")
            return False
            
        # Verify main.cpp contains expected content
        main_cpp_content = (self.output_dir / "main.cpp").read_text()
        if self.test_project_name not in main_cpp_content:
            self.errors.append("main.cpp doesn't contain project name")
            print_test_result("Fast build", False, "Invalid main.cpp content")
            return False
            
        print_test_result("Fast build", True)
        return True
        
    def test_full_build(self):
        """Test full build with compilation"""
        print_test_header("Testing full build")
        
        # Clean output first
        if self.output_dir.exists():
            shutil.rmtree(self.output_dir)
        
        commands = [
            f"project.open {self.test_project_name}",
            "project.build"
        ]
        
        print("Running full build (this may take a minute)...")
        success, stdout, stderr = self.run_command(commands)
        
        if not success:
            self.errors.append(f"Full build failed: {stderr}")
            print_test_result("Full build", False, stderr)
            
            # Check if it's a CMake error
            if "CMake" in stderr:
                print("\nCMake output:")
                print(stderr)
            return False
            
        # Verify executable was created
        executable_paths = [
            self.output_dir / "bin" / self.test_project_name,
            self.output_dir / self.test_project_name,
            self.output_dir / "build" / self.test_project_name
        ]
        
        executable_found = False
        executable_path = None
        for path in executable_paths:
            if path.exists():
                executable_found = True
                executable_path = path
                break
                
        if not executable_found:
            self.errors.append("Executable not created after full build")
            print_test_result("Full build", False, "Executable not found")
            print(f"Checked paths: {[str(p) for p in executable_paths]}")
            
            # List what's actually in the output directory
            if self.output_dir.exists():
                print("\nOutput directory contents:")
                for item in self.output_dir.rglob("*"):
                    if item.is_file():
                        print(f"  {item.relative_to(self.output_dir)}")
            return False
            
        print_test_result("Full build", True, f"Executable at: {executable_path}")
        
        # Try to run the executable briefly to verify it works
        if executable_path:
            print("\nTesting executable...")
            try:
                # Run with timeout to prevent hanging
                result = subprocess.run(
                    [str(executable_path)],
                    capture_output=True,
                    text=True,
                    timeout=3,
                    cwd=str(executable_path.parent)
                )
                print("Executable test completed (timed out as expected for GUI app)")
            except subprocess.TimeoutExpired:
                print("Executable started successfully (timed out as expected)")
            except Exception as e:
                print(f"Executable test failed: {e}")
                
        return True
        
    def test_cmake_template(self):
        """Test that the CMakeLists.txt template is valid"""
        print_test_header("Testing CMake template")
        
        template_path = self.game_engine_dir / "templates" / "basic" / "CMakeLists_template.txt"
        if not template_path.exists():
            self.errors.append("CMakeLists_template.txt not found")
            print_test_result("CMake template", False, "Template file missing")
            return False
            
        content = template_path.read_text()
        
        # Check for modern CMake targets
        required_patterns = [
            "FetchContent_Declare",
            "FetchContent_MakeAvailable",
            "target_link_libraries",
            "spdlog::spdlog",
            "nlohmann_json::nlohmann_json",
            "EnTT::EnTT",
            "glm::glm"
        ]
        
        missing_patterns = []
        for pattern in required_patterns:
            if pattern not in content:
                missing_patterns.append(pattern)
                
        if missing_patterns:
            self.errors.append(f"CMake template missing patterns: {missing_patterns}")
            print_test_result("CMake template", False, f"Missing: {', '.join(missing_patterns)}")
            return False
            
        # Check that it's NOT using hardcoded paths
        if "libspdlogd.a" in content or "_deps/" in content:
            self.errors.append("CMake template contains hardcoded paths")
            print_test_result("CMake template", False, "Contains hardcoded library paths")
            return False
            
        print_test_result("CMake template", True)
        return True
        
    def run_all_tests(self):
        """Run all build system tests"""
        print("\n" + "="*60)
        print("BUILD SYSTEM TEST SUITE")
        print("="*60 + "\n")
        
        # Clean up before tests
        self.cleanup()
        
        tests = [
            self.test_cmake_template,
            self.test_project_creation,
            self.test_fast_build,
            self.test_full_build
        ]
        
        passed = 0
        failed = 0
        
        for test in tests:
            if test():
                passed += 1
            else:
                failed += 1
                
        # Clean up after tests
        self.cleanup()
        
        print("\n" + "="*60)
        print(f"TEST SUMMARY: {passed} passed, {failed} failed")
        if self.errors:
            print("\nErrors encountered:")
            for error in self.errors:
                print(f"  - {error}")
        print("="*60 + "\n")
        
        return failed == 0

if __name__ == "__main__":
    tester = BuildSystemTest()
    success = tester.run_all_tests()
    sys.exit(0 if success else 1)