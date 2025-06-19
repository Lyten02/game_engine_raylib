#!/usr/bin/env python3
"""
TDD GREEN PHASE: Minimal path resolver to fix script execution issues
This provides the minimum functionality to make failing tests pass
"""

import os
import sys
from pathlib import Path

class TestPathResolver:
    """Minimal path resolver for test execution"""
    
    def __init__(self):
        self.current_dir = Path.cwd()
        self.original_cwd = self.current_dir
        
    def find_game_executable(self):
        """Find game_engine executable with intelligent path detection"""
        # Possible executable paths in order of preference
        candidate_paths = [
            # If we're in build/ directory
            "./game_engine",
            # If we're in tests/ directory  
            "../build/game_engine",
            # If we're in GameEngine/ root
            "build/game_engine",
            # If we're elsewhere, search up
            "../game_engine",
            "../../build/game_engine",
            # Absolute fallback
            self._find_build_directory() / "game_engine" if self._find_build_directory() else None
        ]
        
        for path in candidate_paths:
            if path is None:
                continue
                
            path = Path(path)
            if path.exists() and path.is_file() and os.access(path, os.X_OK):
                return str(path.resolve())
        
        return None
    
    def find_script_file(self, script_name):
        """Find script file relative to test directory"""
        # Possible script locations
        candidate_dirs = [
            # If we're in build/, scripts are in ../tests/
            "../tests",
            # If we're in tests/, scripts are in current dir
            ".",
            # If we're elsewhere
            "tests",
            "../GameEngine/tests"
        ]
        
        for dir_path in candidate_dirs:
            script_path = Path(dir_path) / script_name
            if script_path.exists() and script_path.is_file():
                return str(script_path.resolve())
        
        return None
    
    def get_tests_directory(self):
        """Get path to tests directory"""
        current = self.current_dir
        
        # If we're already in tests/
        if current.name == "tests":
            return str(current)
            
        # If we're in build/, tests are at ../tests/
        if current.name == "build":
            tests_dir = current.parent / "tests"
            if tests_dir.exists():
                return str(tests_dir)
        
        # Search up the directory tree
        for parent in current.parents:
            tests_dir = parent / "tests"
            if tests_dir.exists() and tests_dir.is_dir():
                return str(tests_dir)
        
        return None
    
    def get_working_directory_for_executable(self):
        """Get appropriate working directory for running executable"""
        executable_path = self.find_game_executable()
        if not executable_path:
            return str(self.current_dir)
            
        # Executable should be run from its parent directory
        return str(Path(executable_path).parent)
    
    def resolve_test_environment(self):
        """Resolve complete test environment paths"""
        return {
            'executable': self.find_game_executable(),
            'tests_dir': self.get_tests_directory(), 
            'working_dir': self.get_working_directory_for_executable(),
            'current_dir': str(self.current_dir)
        }
    
    def _find_build_directory(self):
        """Find build directory in project structure"""
        current = self.current_dir
        
        # Check current directory
        if current.name == "build":
            return current
            
        # Check siblings
        build_dir = current / "build"
        if build_dir.exists():
            return build_dir
            
        # Check parent's children
        if current.parent:
            build_dir = current.parent / "build"
            if build_dir.exists():
                return build_dir
                
        # Search up
        for parent in current.parents:
            build_dir = parent / "build"
            if build_dir.exists():
                return build_dir
                
        return None

# Global resolver instance
_resolver = None

def get_path_resolver():
    """Get singleton path resolver instance"""
    global _resolver
    if _resolver is None:
        _resolver = TestPathResolver()
    return _resolver

def find_game_executable():
    """Global function to find game executable"""
    return get_path_resolver().find_game_executable()

def find_script_file(script_name):
    """Global function to find script file"""
    return get_path_resolver().find_script_file(script_name)

def get_test_environment():
    """Global function to get test environment"""
    return get_path_resolver().resolve_test_environment()

if __name__ == "__main__":
    # Test the path resolver
    resolver = TestPathResolver()
    env = resolver.resolve_test_environment()
    
    print("Test Environment Resolution:")
    for key, value in env.items():
        print(f"  {key}: {value}")
    
    # Test specific functions
    print(f"\nGame executable: {resolver.find_game_executable()}")
    print(f"Script file (basic_cli_test.txt): {resolver.find_script_file('basic_cli_test.txt')}")
    print(f"Tests directory: {resolver.get_tests_directory()}")