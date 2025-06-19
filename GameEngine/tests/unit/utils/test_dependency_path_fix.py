#!/usr/bin/env python3
"""
TDD GREEN PHASE: Fix dependency path resolution for Python tests
Many Python tests fail because they can't find dependencies in the correct paths
"""

import os
import sys
from pathlib import Path

class DependencyPathResolver:
    """Resolves dependency paths for C++ compilation in tests"""
    
    def __init__(self):
        self.test_dir = Path(__file__).parent
        self.project_root = self.test_dir.parent
        
    def find_deps_directory(self):
        """Find the correct dependencies directory"""
        # Possible dependency locations in order of preference
        candidate_paths = [
            # Cache paths (direct, not in _deps subfolder)
            self.project_root / ".deps_cache",
            # New build system paths
            self.project_root / "build" / "_deps",
            # Cache paths with _deps subfolder
            self.project_root / ".deps_cache" / "_deps",
            # Legacy paths
            self.project_root / "build" / "deps",
            self.project_root / ".deps_cache" / "deps"
        ]
        
        for path in candidate_paths:
            if path.exists() and path.is_dir():
                # Verify it contains expected dependencies
                if self._validate_deps_directory(path):
                    return str(path)
        
        return None
    
    def _validate_deps_directory(self, path):
        """Validate that a directory contains expected dependencies"""
        required_deps = ["raylib-src", "spdlog-src", "entt-src"]
        for dep in required_deps:
            if not (path / dep).exists():
                return False
        return True
    
    def get_include_paths(self):
        """Get include paths for C++ compilation"""
        deps_dir = self.find_deps_directory()
        if not deps_dir:
            return []
            
        deps_path = Path(deps_dir)
        
        include_paths = [
            f"-I{self.project_root}/src",
        ]
        
        # Add dependency includes if they exist
        dep_includes = [
            ("raylib-src", "src"),
            ("spdlog-src", "include"), 
            ("entt-src", "src"),
            ("glm-src", ""),
            ("json-src", "include")
        ]
        
        for dep_name, subdir in dep_includes:
            dep_path = deps_path / dep_name
            if dep_path.exists():
                if subdir:
                    include_path = dep_path / subdir
                    if include_path.exists():
                        include_paths.append(f"-I{include_path}")
                else:
                    include_paths.append(f"-I{dep_path}")
        
        return include_paths
    
    def get_library_paths(self):
        """Get library paths for C++ linking"""
        deps_dir = self.find_deps_directory()
        if not deps_dir:
            return []
            
        deps_path = Path(deps_dir)
        
        lib_paths = [
            f"-L{self.project_root}/build"
        ]
        
        # Add dependency library paths if they exist
        dep_libs = [
            "raylib-build/raylib",
            "spdlog-build"
        ]
        
        for lib_path in dep_libs:
            full_lib_path = deps_path / lib_path
            if full_lib_path.exists():
                lib_paths.append(f"-L{full_lib_path}")
        
        return lib_paths
    
    def get_compilation_flags(self):
        """Get complete compilation flags for C++ tests"""
        includes = " ".join(self.get_include_paths())
        libs = " ".join(self.get_library_paths())
        
        return {
            'includes': includes,
            'libs': libs + " -lraylib -lspdlog",
            'deps_dir': self.find_deps_directory()
        }
    
    def validate_dependencies(self):
        """Validate that all required dependencies are available"""
        deps_dir = self.find_deps_directory()
        if not deps_dir:
            return False, "Dependencies directory not found"
            
        deps_path = Path(deps_dir)
        
        required_deps = [
            "raylib-src",
            "spdlog-src", 
            "entt-src"
        ]
        
        missing_deps = []
        for dep in required_deps:
            if not (deps_path / dep).exists():
                missing_deps.append(dep)
        
        if missing_deps:
            return False, f"Missing dependencies: {', '.join(missing_deps)}"
            
        return True, "All dependencies found"

# Global resolver instance
_dependency_resolver = None

def get_dependency_resolver():
    """Get singleton dependency resolver"""
    global _dependency_resolver
    if _dependency_resolver is None:
        _dependency_resolver = DependencyPathResolver()
    return _dependency_resolver

def get_deps_directory():
    """Global function to get dependencies directory"""
    return get_dependency_resolver().find_deps_directory()

def get_compilation_flags():
    """Global function to get compilation flags"""
    return get_dependency_resolver().get_compilation_flags()

def validate_test_environment():
    """Global function to validate test environment"""
    return get_dependency_resolver().validate_dependencies()

if __name__ == "__main__":
    # Test the dependency resolver
    resolver = DependencyPathResolver()
    
    print("Dependency Path Resolution:")
    print(f"  Dependencies directory: {resolver.find_deps_directory()}")
    
    flags = resolver.get_compilation_flags()
    print(f"  Include flags: {flags['includes']}")
    print(f"  Library flags: {flags['libs']}")
    
    valid, message = resolver.validate_dependencies()
    print(f"  Validation: {message}")
    
    if valid:
        print("✅ Dependencies are properly configured")
    else:
        print("❌ Dependencies need to be set up")