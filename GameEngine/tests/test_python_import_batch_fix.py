#!/usr/bin/env python3
"""
TDD GREEN PHASE: Batch fix for Python import issues
Apply dependency resolver to all failing Python tests
"""

import os
import subprocess
import sys
from pathlib import Path

# Import TDD helpers
try:
    from test_dependency_path_fix import get_compilation_flags, validate_test_environment
    DEPENDENCY_RESOLVER_AVAILABLE = True
except ImportError:
    DEPENDENCY_RESOLVER_AVAILABLE = False

class PythonTestFixer:
    """Fix Python tests with import/dependency issues"""
    
    def __init__(self):
        self.test_dir = Path(__file__).parent
        self.fixed_tests = []
        self.failed_fixes = []
        
    def get_failing_python_tests(self):
        """Get list of Python tests that fail due to import/dependency issues"""
        # From the logs, these are the failing Python tests
        failing_tests = [
            "test_resource_manager_threading.py",
            "test_scene_list_only.py", 
            "test_scene_memory_safety.py",
            "test_script_runner.py",
            "test_security_cli.py",
            "test_test_system_fixes.py",
            "test_utils.py"
        ]
        
        return [test for test in failing_tests if (self.test_dir / test).exists()]
    
    def apply_dependency_resolver_import(self, test_file):
        """Add dependency resolver import to a Python test file"""
        file_path = self.test_dir / test_file
        
        if not file_path.exists():
            return False, f"File {test_file} not found"
            
        try:
            # Read current content
            content = file_path.read_text()
            
            # Check if already has dependency resolver import
            if "test_dependency_path_fix" in content:
                return True, f"{test_file} already has dependency resolver"
            
            # Find the imports section
            lines = content.split('\n')
            import_insert_index = 0
            
            # Find the last import line
            for i, line in enumerate(lines):
                if line.strip().startswith('import ') or line.strip().startswith('from '):
                    import_insert_index = i + 1
                elif line.strip() and not line.strip().startswith('#'):
                    # First non-import, non-comment line
                    break
            
            # Insert dependency resolver import
            dependency_import = """
# Import TDD dependency resolver
try:
    from test_dependency_path_fix import get_compilation_flags, validate_test_environment
except ImportError:
    def get_compilation_flags():
        return {'includes': '', 'libs': '-lraylib -lspdlog', 'deps_dir': None}
    def validate_test_environment():
        return False, "Dependency resolver not available"
"""
            
            lines.insert(import_insert_index, dependency_import)
            
            # Write back
            new_content = '\n'.join(lines)
            file_path.write_text(new_content)
            
            return True, f"Added dependency resolver to {test_file}"
            
        except Exception as e:
            return False, f"Error fixing {test_file}: {e}"
    
    def fix_dependency_paths_in_script(self, test_file):
        """Fix hardcoded dependency paths in script generation"""
        file_path = self.test_dir / test_file
        
        if not file_path.exists():
            return False, f"File {test_file} not found"
            
        try:
            content = file_path.read_text()
            
            # Common problematic patterns to fix
            fixes = [
                # Old dependency paths
                ('if [ -d "../.deps_cache/_deps" ];', 'if [ -d "../.deps_cache" ];'),
                ('DEPS_DIR="../.deps_cache/_deps"', 'DEPS_DIR="../.deps_cache"'),
                ('elif [ -d "../build/_deps" ];', 'elif [ -d "../.deps_cache" ];'),
                ('DEPS_DIR="../build/_deps"', 'DEPS_DIR="../.deps_cache"'),
                
                # Use TDD dependency resolver in shell scripts
                ('echo "Error: Cannot find dependencies directory"', 
                 'echo "Error: Cannot find dependencies directory"\\necho "Searched: ../.deps_cache, ../build/_deps"')
            ]
            
            modified = False
            for old_pattern, new_pattern in fixes:
                if old_pattern in content:
                    content = content.replace(old_pattern, new_pattern)
                    modified = True
            
            if modified:
                file_path.write_text(content)
                return True, f"Fixed dependency paths in {test_file}"
            else:
                return True, f"No dependency path fixes needed in {test_file}"
                
        except Exception as e:
            return False, f"Error fixing paths in {test_file}: {e}"
    
    def validate_test_after_fix(self, test_file):
        """Validate that a test runs without import errors after fixing"""
        file_path = self.test_dir / test_file
        
        try:
            # Quick validation - try to import the test
            result = subprocess.run([
                sys.executable, "-c", f"import sys; sys.path.append('{self.test_dir}'); import {test_file[:-3]}"
            ], capture_output=True, text=True, timeout=5)
            
            return result.returncode == 0, result.stderr
            
        except subprocess.TimeoutExpired:
            return False, "Test import timed out"
        except Exception as e:
            return False, f"Validation error: {e}"
    
    def fix_all_tests(self):
        """Apply fixes to all failing Python tests"""
        failing_tests = self.get_failing_python_tests()
        
        print(f"Found {len(failing_tests)} failing Python tests to fix:")
        for test in failing_tests:
            print(f"  - {test}")
        
        if not DEPENDENCY_RESOLVER_AVAILABLE:
            print("❌ Dependency resolver not available - creating minimal fixes only")
        
        print("\nApplying fixes...")
        
        for test_file in failing_tests:
            print(f"\nFixing {test_file}:")
            
            # Apply dependency resolver import
            success, message = self.apply_dependency_resolver_import(test_file)
            print(f"  Import fix: {message}")
            
            # Fix dependency paths in scripts
            success2, message2 = self.fix_dependency_paths_in_script(test_file)
            print(f"  Path fix: {message2}")
            
            if success and success2:
                self.fixed_tests.append(test_file)
                print(f"  ✅ {test_file} fixed")
            else:
                self.failed_fixes.append(test_file)
                print(f"  ❌ {test_file} fix failed")
        
        print(f"\n📊 Fix Results:")
        print(f"  ✅ Fixed: {len(self.fixed_tests)} tests")
        print(f"  ❌ Failed: {len(self.failed_fixes)} tests")
        
        if self.fixed_tests:
            print(f"\nFixed tests:")
            for test in self.fixed_tests:
                print(f"  - {test}")
        
        if self.failed_fixes:
            print(f"\nFailed fixes:")
            for test in self.failed_fixes:
                print(f"  - {test}")

if __name__ == "__main__":
    print("🛠️  TDD GREEN PHASE: Batch fixing Python import issues")
    print("=" * 60)
    
    fixer = PythonTestFixer()
    fixer.fix_all_tests()
    
    print("\n✅ Batch fix complete!")
    print("Re-run failing tests to verify fixes work.")