# CLAUDE.md

This file provides guidance to Claude Code when working with this ECS GameEngine repository.

## Quick Reference Commands

```bash
# 🚀 Most used commands (copy-paste ready)
cd GameEngine && ./rebuild_smart.sh        # Smart rebuild
make test                                   # Run all tests (~7s)
make test-parallel                         # Parallel tests (~6.5s) 
sh full_test_fixed.sh                      # Full project test suite
./game --json -c "project.list"           # List projects with JSON output
./game --headless --script test.txt       # Run test script
```

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
   make test-parallel                   # Verify nothing breaks
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
./game --json -c "command"

# ✅ GOOD: Use structured error handling
CLIResult result = command.execute();
if (!result.success) { /* handle error */ }
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
# Use parallel testing by default (13% faster)
make test-parallel          # ~6.5s vs 7.0s sequential

# For debugging specific tests
python3 tests/test_name.py --verbose    # Real-time errors
grep "TEST FAILED" test_log_*.log       # Find failures in logs
```

### 💾 Dependency Cache Management
```bash
# ⚠️ NEVER delete these unless absolutely necessary:
# build/_deps/              # Main cache (~60s rebuild if deleted)
# output/*/build/_deps/     # Per-project cache

# 🚀 Speed up slow builds:
export FETCHCONTENT_FULLY_DISCONNECTED=TRUE
make test

# 🧹 Clean specific project (keeps cache):
rm -rf ../output/TestProject/{bin,CMakeFiles}
# Keep: ../output/TestProject/build/_deps/
```

## Common Workflows

### 🎮 Game Development Cycle
```bash
# Standard project workflow
./game -c "project.create MyGame"     # Create new game
./game -c "scene.create main"         # Create main scene  
./game -c "entity.create Player"      # Add entities
./game -c "project.build"             # Build release
./game -c "project.run"               # Test the game

# Development mode with hot-reload
./game                                # Interactive mode
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

**Parallel tests failing randomly:**
```bash
# Use sequential for debugging
make test
# Check resource conflicts in logs
```

### 📊 Performance Expectations
- **First build**: ~60s (building dependencies)
- **Cached builds**: ~3s (using cached deps)  
- **Sequential tests**: ~7.0s
- **Parallel tests**: ~6.5s (13% improvement)
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
make test-parallel && sh full_test_fixed.sh

# Security verification  
python3 tests/test_security_cli.py
python3 tests/test_command_injection_simple.py

# Performance check
time make test-parallel  # Should be ~6.5s
```

---
**Remember**: This is a production game engine. Always prioritize **security**, **testing**, and **performance**. When in doubt, use **"think harder"** to ensure robust solutions.