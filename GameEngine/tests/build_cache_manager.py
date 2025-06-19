#!/usr/bin/env python3
"""
TDD GREEN PHASE: Minimal implementation of build cache manager
This provides the minimum functionality to make tests pass
"""

import os
import json
import hashlib
import time
from pathlib import Path

class BuildCacheManager:
    """Minimal build cache manager to optimize test_fast_build.py"""
    
    def __init__(self, project_name):
        self.project_name = project_name
        self.project_dir = Path(f"../output/{project_name}")
        self.cache_dir = self.project_dir / ".build_cache"
        self.cache_dir.mkdir(exist_ok=True)
        
        # Cache files
        self.hash_file = self.cache_dir / "build_hash.json"
        self.deps_file = self.cache_dir / "deps_manifest.json"
        self.time_file = self.cache_dir / "last_build_time.txt"
    
    def is_cache_valid(self):
        """Check if build cache is valid - minimal implementation"""
        try:
            # If no cache files exist, cache is invalid
            if not all(f.exists() for f in [self.hash_file, self.deps_file, self.time_file]):
                return False
            
            # Load cached hash
            with open(self.hash_file, 'r') as f:
                cached_data = json.load(f)
            
            # Calculate current hash of dependencies
            current_hash = self._calculate_dependencies_hash()
            
            # Cache is valid if hashes match
            return cached_data.get('deps_hash') == current_hash
            
        except (FileNotFoundError, json.JSONDecodeError, KeyError):
            return False
    
    def update_cache(self):
        """Update build cache after successful build"""
        try:
            # Calculate current dependency hash
            deps_hash = self._calculate_dependencies_hash()
            
            # Save hash data
            hash_data = {
                'deps_hash': deps_hash,
                'project_name': self.project_name,
                'cache_updated': time.time()
            }
            
            with open(self.hash_file, 'w') as f:
                json.dump(hash_data, f, indent=2)
            
            # Save dependencies manifest
            deps_manifest = self._get_dependencies_manifest()
            with open(self.deps_file, 'w') as f:
                json.dump(deps_manifest, f, indent=2)
            
            # Save build time
            with open(self.time_file, 'w') as f:
                f.write(str(time.time()))
                
        except Exception as e:
            print(f"Warning: Failed to update build cache: {e}")
    
    def invalidate_cache(self):
        """Invalidate build cache"""
        try:
            for cache_file in [self.hash_file, self.deps_file, self.time_file]:
                if cache_file.exists():
                    cache_file.unlink()
        except Exception as e:
            print(f"Warning: Failed to invalidate cache: {e}")
    
    def _calculate_dependencies_hash(self):
        """Calculate hash of project dependencies - minimal implementation"""
        try:
            # Hash based on CMakeLists.txt and main source files
            hash_inputs = []
            
            # Include CMakeLists.txt if it exists
            cmake_file = self.project_dir / "CMakeLists.txt"
            if cmake_file.exists():
                hash_inputs.append(cmake_file.read_text())
            
            # Include main.cpp if it exists
            main_file = self.project_dir / "main.cpp"
            if main_file.exists():
                hash_inputs.append(main_file.read_text())
            
            # Include game_config.json if it exists
            config_file = self.project_dir / "game_config.json"
            if config_file.exists():
                hash_inputs.append(config_file.read_text())
            
            # Combine all inputs and hash
            combined = ''.join(hash_inputs)
            return hashlib.sha256(combined.encode()).hexdigest()
            
        except Exception:
            # Return consistent hash for empty/error case
            return "empty_project_hash"
    
    def _get_dependencies_manifest(self):
        """Get list of project dependencies - minimal implementation"""
        return {
            'project_name': self.project_name,
            'dependencies': ['raylib', 'glfw', 'glm'],  # Standard deps
            'generated_at': time.time()
        }

def is_cache_valid(project_name):
    """Global function for cache validity check"""
    cache_manager = BuildCacheManager(project_name)
    return cache_manager.is_cache_valid()

def update_build_cache(project_name):
    """Global function to update build cache"""
    cache_manager = BuildCacheManager(project_name)
    cache_manager.update_cache()

def invalidate_build_cache(project_name):
    """Global function to invalidate build cache"""
    cache_manager = BuildCacheManager(project_name)
    cache_manager.invalidate_cache()

if __name__ == "__main__":
    # Test the cache manager
    test_project = "CacheTestProject"
    manager = BuildCacheManager(test_project)
    
    print(f"Cache valid: {manager.is_cache_valid()}")
    manager.update_cache()
    print(f"Cache valid after update: {manager.is_cache_valid()}")
    manager.invalidate_cache()
    print(f"Cache valid after invalidation: {manager.is_cache_valid()}")