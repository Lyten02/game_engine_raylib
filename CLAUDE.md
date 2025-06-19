# CLAUDE.md

This file provides guidance to Claude Code when working with this ECS GameEngine repository.

## Quick Reference Commands

# 🚀 Most used commands (copy-paste ready)
cd GameEngine && ./rebuild_smart.sh        # Smart rebuild
make test                                   # Run all tests (~7s) 
sh full_test_fixed.sh                      # Full project test suite
./game_engine --json -c "project.list"    # List projects with JSON output
./game_engine --headless --script test.txt # Run test script

# Rebuild options (from fastest to slowest)
cd GameEngine

# Smart rebuild - automatically chooses best strategy
./rebuild_smart.sh

# Incremental rebuild - only recompiles changed files (fastest)
./rebuild_incremental.sh  

# Fast rebuild - preserves cached dependencies
./rebuild_fast.sh

# Full clean rebuild - removes everything including deps
./rebuild.sh

# Manual build
mkdir -p build
cd build
cmake ..
make -j8
./game_engine

# CMake targets
make test              # Run all Python and script tests
make test-fast         # Run tests without rebuild
make test-cpp          # Run C++ tests only
make all-tests         # Run all tests (Python + Script + C++)
make test-unit         # Run unit tests only
make test-integration  # Run integration tests only
make test-system       # Run system tests only
make clean-tests       # Clean test output
make clean-logs        # Remove log files
make clean-all         # Full clean
```

## Testing Commands

```bash
# Run all tests from build directory
make test              # Run Python and script tests
make test-cpp          # Run C++ tests with auto-compilation
make all-tests         # Run complete test suite

# Run tests by category
make test-unit         # Unit tests only
make test-integration  # Integration tests only
make test-system       # System tests only

# CLI testing
./game_engine --json --script ../tests/basic_cli_test.txt
./game_engine --headless --batch "project.create test" "project.build"

# Test logging and debugging
# After running tests, check the generated log files:
# - test_log_YYYYMMDD_HHMMSS.log - Detailed execution log with full error output
# - test_results.json - Structured test results

# Finding test failures in logs:
grep "TEST FAILED" test_log_*.log
grep -A 50 "TEST FAILED: test_name.py" test_log_*.log
```

## Testing System

The test suite uses automatic test discovery from organized directories:

### Test Directory Structure:
```
tests/
├── unit/              # Unit tests
├── integration/       # Integration tests  
├── system/           # System tests
├── fixtures/         # Test data
├── utils/           # Test utilities
└── tools/           # Test tools
```

### 🔗 Related Test System Files:
When modifying the test system, these files must be updated together:
1. **CMakeLists.txt** - Test target definitions (make test, test-cpp, etc.)
2. **cmake/TestDiscovery.cmake** - Automatic test discovery logic
3. **tests/README.md** - Test documentation

The test runner automatically finds:
- Python tests: `test_*.py` files
- C++ tests: `test_*.cpp` files  
- Script tests: `test*.txt` or `*_test.txt` files

### Test Categories (old system - being phased out):
1. **LIGHTWEIGHT**: Fast unit tests, minimal resources
2. **BUILD**: Build system tests, moderate resources
3. **HEAVY**: Resource-intensive tests (memory, config stress)
4. **COMMAND**: CLI command tests, very fast
5. **SCRIPT**: Script execution tests

## Development Workflow

### 🔄 TDD-First Approach (MANDATORY)
**Always follow Test-Driven Development:**

1. **🔴 RED**: Write failing tests FIRST
   ```bash
   # Write tests before any implementation
   python3 tests/test_your_feature.py  # Should fail
   make test-cpp                        # C++ tests should fail
   ```

2. **🟢 GREEN**: Write minimal code to pass tests
   ```bash
   # Implement just enough to make tests pass
   make test && make test-cpp           # All tests should pass
   ```

3. **🔵 REFACTOR**: Improve code while keeping tests green
   ```bash
   make test                            # Verify nothing breaks
   ```

### 🎯 Problem-Solving Strategy
Use progressive thinking levels based on complexity:

- **"think"** - Simple bug fixes, routine changes
- **"think hard"** - Architecture decisions, complex debugging  
- **"think harder"** - Performance optimization, security issues
- **"ultrathink"** - Critical system redesign, complex refactoring

### 📋 Before Starting Any Task
**ALWAYS create a plan first:**
```
**Think hard** and create implementation plan:
1. Analyze existing code structure
2. Identify files to modify  
3. Write test cases first (TDD)
4. Implement minimal changes
5. Verify all tests pass
6. Refactor if needed
```

## Code Style & Standards

### 🎨 C++ Style Guide
```cpp
// ✅ GOOD: Use descriptive names
class ResourceManager {
    void loadTexture(const std::string& filename);
    std::shared_ptr<Texture> getTexture(const std::string& name);
};

// ✅ GOOD: Use const references for parameters
void createEntity(const std::string& name, const TransformComponent& transform);

// ❌ AVOID: Raw pointers, use smart pointers
// Texture* loadTexture(); // NO
std::shared_ptr<Texture> loadTexture(); // YES
```

### 📝 JSON/CLI Standards
```bash
# ✅ GOOD: Always provide JSON output for automation
./game_engine --json -c "command"

# ✅ GOOD: Use structured error handling
CLIResult result = command.execute();
if (!result.success) { /* handle error */ }

# Or use fast build command directly
./game_engine --headless -c "project.build.fast"
```

### 🔒 Security Requirements (CRITICAL)
```cpp
// ✅ ALWAYS use ProcessExecutor, NEVER std::system()
ProcessExecutor executor;
executor.execute(command, args, workingDir);  // SAFE

// ❌ NEVER use std::system() - command injection risk!
// std::system(userInput);  // DANGEROUS!

// ✅ ALWAYS validate paths
if (!ProcessExecutor::sanitizePath(userPath)) {
    return error("Invalid path");
}
```

## Build System Mastery

### 🏗️ Build Commands Priority Order
```bash
# 1. Smart rebuild (recommended for most changes)
./rebuild_smart.sh           # Auto-detects best strategy

# 2. Incremental rebuild (fastest for small changes)  
./rebuild_incremental.sh     # Only changed files

# 3. Fast rebuild (when dependencies might have changed)
./rebuild_fast.sh           # Preserves cache

# 4. Full rebuild (when everything is broken)
./rebuild.sh                # Nuclear option
```

### ⚡ Performance Testing
```bash
# Run tests
make test                   # ~7.0s

# For debugging specific tests
python3 tests/test_name.py --verbose    # Real-time errors
grep "TEST FAILED" test_log_*.log       # Find failures in logs
```

### 💾 Dependency Cache Management
```bash
# ⚠️ NEVER delete these unless absolutely necessary:
# .deps_cache/              # Global dependency cache (~60s rebuild if deleted)
# build/CMakeCache.txt      # CMake configuration cache

# 🚀 Speed up slow builds:
export FETCHCONTENT_FULLY_DISCONNECTED=TRUE
make test

# 🧹 Clean specific project (keeps cache):
rm -rf ../output/TestProject/{bin,CMakeFiles}
# Keep dependency cache intact

# 📊 Cache performance:
# - First build (no cache): ~60-100s
# - Fast rebuild (with cache): ~17s
# - Incremental rebuild: ~3s
```

## Common Workflows

### 🎮 Game Development Cycle
```bash
# Interactive console (F1 in-game or ` key)
./game_engine

# Single command
./game_engine --command "project.list"
./game_engine --json -c "entity.list"

# Batch commands
./game_engine --batch "project.create MyGame" "scene.create main" "project.build"

# Script execution
./game_engine --script build_commands.txt
./game_engine --headless --script ../tests/test_script.txt

# Standard project workflow
./game_engine -c "project.create MyGame"     # Create new game
./game_engine -c "project.open MyGame"       # Open existing project
./game_engine -c "scene.create main"         # Create main scene  
./game_engine -c "entity.create Player"      # Add entities
./game_engine -c "project.build"             # Build release
./game_engine -c "project.run"               # Test the game

# Development mode with hot-reload
./game_engine                                # Interactive mode
# Press F5 (play), F6 (pause), ` (console)
```

### 🐛 Debugging Workflow
```bash
# 1. Check recent logs
ls -la logs/test_log_*.log

# 2. Find specific failures  
grep -A 50 "TEST FAILED: test_name" test_log_*.log

# 3. Run single test with verbose output
python3 tests/test_name.py -v

# 4. Check JSON results
cat test_results.json | jq '.failed_tests'
```

### 🔍 Code Investigation 
```bash
# Search architecture patterns
grep -r "class.*Component" src/        # Find all components
grep -r "System.*::" src/               # Find all systems  
grep -r "CLICommand" src/               # Find CLI commands

# Check security implementations
grep -r "ProcessExecutor" src/          # Safe command execution
grep -r "std::system" src/              # Should return NO results!
```

## Architecture Understanding

### 🏗️ ECS Core Components
```
Engine (60 FPS loop) 
├── EngineCore (shared CLI/GUI logic)
├── Scene (EnTT registry + lifecycle)  
├── SystemsManager (system initialization)
└── CLIEngine (headless command mode)
```

### 🎯 Key Integration Points
- **EnTT 3.13.2**: ECS framework core
- **RayLib 5.0**: Rendering & input  
- **ProcessExecutor**: SECURE command execution
- **nlohmann/json**: CLI serialization
- **spdlog**: Debug-level logging

### 🔧 When Adding New Features
1. **Follow ECS pattern**: Data in components, logic in systems
2. **Add CLI command**: Register in appropriate CommandRegistry category
3. **Support headless mode**: Works without GUI
4. **JSON output**: For automation and testing  
5. **Write tests**: Both Python (CLI) and C++ (core logic)
6. **Security review**: Use ProcessExecutor, validate inputs

## Troubleshooting Guide

### 🚨 Common Issues & Solutions

**Build fails with dependency errors:**
```bash
# Clean and rebuild cache
make clean-all && ./rebuild.sh
```

**Tests are slow (>2 minutes):**
```bash
# Check if cache exists
ls -la build/_deps/
# If missing, first run will be slow (rebuilding cache)
```

**Command injection security test fails:**
```bash
# Check all std::system() usage has been replaced
grep -r "std::system" src/  # Should be empty!
```


### 📊 Performance Expectations
- **First build**: ~60s (building dependencies)
- **Cached builds**: ~3s (using cached deps)  
- **Tests**: ~7.0s
- **Full test suite**: ~30-60s total

## Integration with Claude Code

### 💡 Best Practices for AI Collaboration

**When asking for code changes:**
```
**Think hard** and implement [feature] using TDD:
1. Analyze existing architecture in src/
2. Write failing tests first
3. Implement minimal code to pass tests  
4. Ensure security (use ProcessExecutor)
5. Update CLI if needed (with JSON support)
6. Run full test suite
```

**For complex architecture changes:**
```
**Ultrathink** and redesign [system]:
1. Review current ECS architecture  
2. Plan component/system changes
3. Consider backward compatibility
4. Write comprehensive test coverage
5. Implement incrementally
6. Validate performance impact
```

### 🔄 Continuous Integration
```bash
# Run before every commit
make test && sh full_test_fixed.sh

# Security verification  
python3 tests/test_security_cli.py
python3 tests/test_command_injection_simple.py

# Performance check
time make test  # Should be ~7.0s
```

---
**Remember**: This is a production game engine. Always prioritize **security**, **testing**, and **performance**. When in doubt, use **"think harder"** to ensure robust solutions.