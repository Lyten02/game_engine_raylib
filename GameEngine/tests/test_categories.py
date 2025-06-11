#!/usr/bin/env python3
"""
Test categorization system for parallel execution
"""

from enum import Enum
from typing import List, Dict, Set
from pathlib import Path

class TestCategory(Enum):
    """Test categories based on resource usage and execution characteristics"""
    LIGHTWEIGHT = "lightweight"  # Fast, minimal resources, high parallelism
    BUILD = "build"              # Build operations, moderate parallelism 
    HEAVY = "heavy"              # Slow, resource-intensive, sequential
    COMMAND = "command"          # CLI command tests, fast, parallel
    SCRIPT = "script"           # Script execution tests, parallel

class TestCategorizer:
    """Categorizes tests based on their characteristics"""
    
    def __init__(self):
        self.categories = {
            TestCategory.LIGHTWEIGHT: {
                # Fast tests with minimal resource usage
                "test_cli_basic.py",
                "test_cli_json.py", 
                "test_cli_output.py",
                "test_command_performance.py",
                "test_command_timeout.py",
                "test_config_cli_validation.py",
                "test_config_loading_paths.py",
                "test_config_system.py",
                "test_editor_mode_build.py",
                "test_entity_create_returns_id.py",
                "test_headless_resource_manager.py",
                "test_log_limiting.py",
                "test_log_reduction.py",
                "test_logging.py",
                "test_progress_bar.py",
                "test_project_utils.py",
                "test_projects.py",
                "test_resource_functionality.py",
                "test_scene_list_only.py",
                "test_script_runner.py",
                "test_security_cli.py",
                "test_sync_build.py",
                "test_utils.py",
            },
            
            TestCategory.BUILD: {
                # Build-related tests that may share dependencies
                "test_build_output.py",
                "test_build_system.py",
                "test_build_templates.py",
                "test_fast_build.py",
            },
            
            TestCategory.HEAVY: {
                # Resource-intensive tests best run sequentially
                "test_config_edge_cases.py",    # Complex config testing
                "test_config_loading.py",       # File I/O intensive
                "test_config_stress.py",        # Stress testing
                "test_full_workflow.py",        # End-to-end workflow
                "test_resource_manager_memory.py",  # C++ compilation
                "test_resource_manager_threading.py",  # Threading tests
                "test_scene_memory_safety.py",  # Memory testing
            }
        }
        
        # Script tests are usually lightweight
        self.script_test_patterns = {
            "*.txt": TestCategory.SCRIPT
        }
        
        # Command tests are lightweight
        self.command_tests = [
            ("Help", "help"),
            ("Project List", "project.list"),
            ("Invalid Command", "invalid.command", False),  # Expected to fail
            ("Engine Info", "engine.info")
        ]
    
    def categorize_test(self, test_path: Path) -> TestCategory:
        """Categorize a single test file"""
        test_name = test_path.name
        
        # Check each category
        for category, test_set in self.categories.items():
            if test_name in test_set:
                return category
        
        # Handle script tests
        if test_path.suffix == ".txt":
            return TestCategory.SCRIPT
            
        # Default to lightweight for unknown tests
        return TestCategory.LIGHTWEIGHT
    
    def get_tests_by_category(self, test_dir: Path, skip_full_build: bool = False) -> Dict[TestCategory, List[Path]]:
        """Get all tests organized by category"""
        categorized_tests = {category: [] for category in TestCategory}
        
        # Python tests
        python_tests = list(test_dir.glob("test_*.py"))
        exclude_tests = ["test_build_system_fixed.py", "test_build_system_simple.py"]
        
        # In fast mode, exclude heavy build tests
        if skip_full_build:
            exclude_tests.extend(["test_fast_build.py", "test_full_workflow.py", "test_build_full.py"])
        
        python_tests = [t for t in python_tests if t.name not in exclude_tests]
        
        for test_path in python_tests:
            category = self.categorize_test(test_path)
            categorized_tests[category].append(test_path)
        
        # Script tests
        script_tests = list(test_dir.glob("*.txt"))
        script_tests = [t for t in script_tests if t.name not in ["CMakeCache.txt", "CMakeLists.txt"]]
        categorized_tests[TestCategory.SCRIPT].extend(script_tests)
        
        # Command tests are handled separately in the runner
        
        return categorized_tests
    
    def get_parallel_groups(self, test_dir: Path, skip_full_build: bool = False) -> Dict[str, Dict]:
        """Get tests organized into parallel execution groups"""
        categorized = self.get_tests_by_category(test_dir, skip_full_build)
        
        return {
            "parallel_fast": {
                "tests": categorized[TestCategory.LIGHTWEIGHT] + categorized[TestCategory.SCRIPT],
                "max_workers": 4,
                "description": "Fast parallel tests"
            },
            "parallel_build": {
                "tests": categorized[TestCategory.BUILD],
                "max_workers": 2,
                "description": "Build tests with coordination"
            },
            "sequential_heavy": {
                "tests": categorized[TestCategory.HEAVY],
                "max_workers": 1,
                "description": "Heavy tests (sequential)"
            },
            "commands": {
                "tests": self.command_tests,
                "max_workers": 4,
                "description": "CLI command tests"
            }
        }
    
    def estimate_execution_time(self, test_dir: Path, skip_full_build: bool = False) -> Dict[str, float]:
        """Estimate execution times for each group (in seconds)"""
        groups = self.get_parallel_groups(test_dir, skip_full_build)
        
        # Rough estimates based on current test performance
        base_estimates = {
            "parallel_fast": len(groups["parallel_fast"]["tests"]) * 0.3,  # ~0.3s per test
            "parallel_build": len(groups["parallel_build"]["tests"]) * 5,   # ~5s per test (skipped)
            "sequential_heavy": len(groups["sequential_heavy"]["tests"]) * 10, # ~10s per test  
            "commands": len(groups["commands"]["tests"]) * 0.1  # ~0.1s per test
        }
        
        # Adjust for parallel execution
        estimates = {}
        for group_name, group_info in groups.items():
            if group_name in base_estimates:
                max_workers = group_info["max_workers"]
                test_count = len(group_info["tests"])
                
                if max_workers > 1 and test_count > 0:
                    # Parallel speedup with some overhead
                    effective_workers = min(max_workers, test_count)
                    speedup = effective_workers * 0.85  # 85% efficiency
                    estimates[group_name] = base_estimates[group_name] / speedup
                else:
                    estimates[group_name] = base_estimates[group_name]
        
        return estimates

def main():
    """Demo the categorization system"""
    categorizer = TestCategorizer()
    test_dir = Path(__file__).parent
    
    print("🏷️  GameEngine Test Categorization")
    print("=" * 50)
    
    groups = categorizer.get_parallel_groups(test_dir, skip_full_build=True)
    estimates = categorizer.estimate_execution_time(test_dir, skip_full_build=True)
    
    total_tests = 0
    total_time = 0
    
    for group_name, group_info in groups.items():
        test_count = len(group_info["tests"])
        estimated_time = estimates.get(group_name, 0)
        total_tests += test_count
        total_time += estimated_time
        
        print(f"\n{group_info['description']}:")
        print(f"  Tests: {test_count}")
        print(f"  Workers: {group_info['max_workers']}")
        print(f"  Estimated time: {estimated_time:.1f}s")
        
        if test_count <= 10:  # Show test names for smaller groups
            for test in group_info["tests"][:5]:
                if isinstance(test, Path):
                    print(f"    - {test.name}")
                else:
                    print(f"    - {test}")
            if test_count > 5:
                print(f"    ... and {test_count - 5} more")
    
    print(f"\n📊 Summary:")
    print(f"  Total tests: {total_tests}")
    print(f"  Estimated total time: {total_time:.1f}s")
    print(f"  vs Sequential time: ~{total_tests * 3:.1f}s")
    print(f"  Potential speedup: {(total_tests * 3) / total_time:.1f}x")

if __name__ == "__main__":
    main()