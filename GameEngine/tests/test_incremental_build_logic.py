#!/usr/bin/env python3
"""
TDD RED PHASE: Failing tests for incremental build logic
These tests define the incremental build behavior that should replace full builds
"""

import unittest
import subprocess
import time
import os
import json
from cleanup_utils import TestProjectCleaner

class TestIncrementalBuildLogic(unittest.TestCase):
    """Tests for incremental build logic - currently FAILING"""
    
    def setUp(self):
        """Setup test environment"""
        self.game_exe = "./game_engine"
        self.test_project = "IncrementalBuildTest"
        
    def test_incremental_build_exists(self):
        """FAILING: Test that incremental build command exists"""
        with TestProjectCleaner(self.test_project):
            # Create project
            subprocess.run([
                self.game_exe, "--headless", "-c", f"project.create {self.test_project}"
            ], capture_output=True, text=True)
            
            subprocess.run([
                self.game_exe, "--headless", "-c", f"project.open {self.test_project}"
            ], capture_output=True, text=True)
            
            # This should FAIL because project.build.incremental doesn't exist
            result = subprocess.run([
                self.game_exe, "--headless", "-c", "project.build.incremental"
            ], capture_output=True, text=True)
            
            # Expected: command should exist and succeed
            self.assertEqual(result.returncode, 0, "project.build.incremental should exist")
    
    def test_incremental_build_speed(self):
        """FAILING: Test that incremental build is significantly faster"""
        with TestProjectCleaner(self.test_project):
            # Create and do initial full build
            subprocess.run([
                self.game_exe, "--headless", "-c", f"project.create {self.test_project}"
            ], capture_output=True, text=True)
            
            subprocess.run([
                self.game_exe, "--headless", "-c", f"project.open {self.test_project}"
            ], capture_output=True, text=True)
            
            # Full build first
            start = time.time()
            subprocess.run([
                self.game_exe, "--headless", "-c", "project.build"
            ], capture_output=True, text=True)
            full_build_time = time.time() - start
            
            # This should FAIL because incremental build logic doesn't exist
            start = time.time()
            result = subprocess.run([
                self.game_exe, "--headless", "-c", "project.build.incremental"
            ], capture_output=True, text=True)
            incremental_time = time.time() - start
            
            # Expected: incremental should be under 5 seconds
            self.assertLess(incremental_time, 5.0, "Incremental build should be under 5 seconds")
            self.assertEqual(result.returncode, 0)
    
    def test_build_cache_directory_structure(self):
        """FAILING: Test that build cache has proper directory structure""" 
        with TestProjectCleaner(self.test_project):
            # Create project
            subprocess.run([
                self.game_exe, "--headless", "-c", f"project.create {self.test_project}"
            ], capture_output=True, text=True)
            
            subprocess.run([
                self.game_exe, "--headless", "-c", f"project.open {self.test_project}"
            ], capture_output=True, text=True)
            
            # Run incremental build
            subprocess.run([
                self.game_exe, "--headless", "-c", "project.build.incremental"  
            ], capture_output=True, text=True)
            
            # This should FAIL because cache structure doesn't exist
            cache_dir = f"../output/{self.test_project}/.build_cache"
            
            # Expected cache structure
            expected_files = [
                f"{cache_dir}/build_hash.json",
                f"{cache_dir}/deps_manifest.json", 
                f"{cache_dir}/last_build_time.txt"
            ]
            
            for expected_file in expected_files:
                self.assertTrue(os.path.exists(expected_file), 
                               f"Cache file should exist: {expected_file}")
    
    def test_dependency_hash_calculation(self):
        """FAILING: Test that dependency hashes are calculated correctly"""
        with TestProjectCleaner(self.test_project):
            # Create project with known dependencies
            subprocess.run([
                self.game_exe, "--headless", "-c", f"project.create {self.test_project}"
            ], capture_output=True, text=True)
            
            # This should FAIL because hash calculation doesn't exist
            deps_hash = self._calculate_deps_hash(self.test_project)
            
            # Expected: should return consistent hash for same dependencies
            self.assertIsNotNone(deps_hash)
            self.assertIsInstance(deps_hash, str)
            self.assertGreater(len(deps_hash), 0)
            
            # Hash should be consistent
            deps_hash2 = self._calculate_deps_hash(self.test_project)
            self.assertEqual(deps_hash, deps_hash2)
    
    def test_source_file_modification_detection(self):
        """FAILING: Test that source file modifications are detected"""
        with TestProjectCleaner(self.test_project):
            # Create project
            subprocess.run([
                self.game_exe, "--headless", "-c", f"project.create {self.test_project}"
            ], capture_output=True, text=True)
            
            subprocess.run([
                self.game_exe, "--headless", "-c", f"project.open {self.test_project}"
            ], capture_output=True, text=True)
            
            # Get initial modification time
            initial_mtime = self._get_project_modification_time(self.test_project)
            
            # Modify source file
            project_path = f"../output/{self.test_project}"
            if os.path.exists(f"{project_path}/main.cpp"):
                with open(f"{project_path}/main.cpp", "a") as f:
                    f.write("// Test modification\n")
            
            # This should FAIL because modification detection doesn't exist
            new_mtime = self._get_project_modification_time(self.test_project)
            
            # Expected: modification time should be different
            self.assertNotEqual(initial_mtime, new_mtime)

    # Helper methods that will FAIL because functionality doesn't exist yet
    def _calculate_deps_hash(self, project_name):
        """Calculate dependency hash - WILL FAIL"""
        # This function doesn't exist yet
        return None
    
    def _get_project_modification_time(self, project_name):
        """Get project modification time - WILL FAIL"""
        # This function doesn't exist yet  
        return 0

if __name__ == "__main__":
    print("🔴 Running FAILING tests for incremental build logic...")
    print("These tests define the incremental build behavior we need to implement")
    unittest.main(verbosity=2)