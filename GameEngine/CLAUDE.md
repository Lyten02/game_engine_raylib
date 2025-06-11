# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
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

# Test all project in build folder
# Testing and logging in file logs/
sh full_test_fixed.sh

# Manual build
mkdir -p build
cd build
cmake ..
make -j8
./game

# CMake targets
make test              # Run all tests (sequential)
make test-fast         # Run tests without full builds
make test-parallel     # Run tests in parallel (automatic worker count)
make test-parallel-2   # Run tests with 2 workers
make test-parallel-4   # Run tests with 4 workers  
make test-parallel-8   # Run tests with 8 workers
make test-cpp          # Run C++ tests only
make clean-tests       # Clean test projects
make clean-logs        # Remove log files
make clean-all         # Full clean
```

## Testing Commands

```bash
# Run all tests from build directory
make test              # Sequential execution (~7s)
make test-parallel     # Parallel execution (~6.5s, 13% faster)
make test-parallel-4   # Force 4 workers

# Run specific test suites
python3 ../tests/run_all_tests.py              # All Python tests
python3 ../tests/run_all_tests.py --parallel   # Parallel mode
python3 ../tests/run_all_tests.py --parallel --workers 2
python3 ../tests/test_cli_basic.py             # Specific test
./tests/compile_and_run_tests.sh               # C++ ResourceManager tests

# CLI testing
./game --json --script ../tests/basic_cli_test.txt
./game --headless --batch "project.create test" "project.build"
```

## Parallel Testing System

The test suite now supports parallel execution for improved performance:

### How it works:
- Tests are categorized by resource usage and execution characteristics
- Lightweight tests run with high parallelism (4 workers)
- Build tests run with moderate parallelism (2 workers)
- Heavy tests run sequentially to avoid resource contention
- Command tests run in parallel for quick validation

### Performance:
- Sequential: ~7.0 seconds
- Parallel: ~6.5 seconds (13% improvement)
- Greater improvements expected on systems with more cores

### Test Categories:
1. **LIGHTWEIGHT**: Fast unit tests, minimal resources
2. **BUILD**: Build system tests, moderate resources
3. **HEAVY**: Resource-intensive tests (memory, config stress)
4. **COMMAND**: CLI command tests, very fast
5. **SCRIPT**: Script execution tests

## Test Caching System

The test system uses cached dependencies to speed up builds significantly (from ~60s to ~3s).

### Speed up slow builds:
If builds are taking too long even with cache, try:
```bash
# Use environment variable to skip git updates
export FETCHCONTENT_FULLY_DISCONNECTED=TRUE
make test

# Or use fast build command directly
./game --headless -c "project.build.fast"
```

### How caching works:
1. **First test run**: Downloads and builds dependencies (~60 seconds)
   - Creates `_deps/` directories with compiled libraries
   - Caches raylib, glfw, spdlog, etc.

2. **Subsequent runs**: Uses cached dependencies (~3 seconds)
   - `project.build` - Full build from scratch (slow)
   - `project.build.fast` - Fast build using cache (fast)
   - Tests automatically choose the right command

### Important directories:
- `build/_deps/` - Main dependency cache
- `output/*/build/_deps/` - Per-project dependency cache
- **DO NOT DELETE** these directories unless you want full rebuild

### Best practices for fast tests:
```bash
# First run (builds cache)
make test  # Takes ~2-3 minutes total

# Subsequent runs (uses cache)
make test  # Takes ~30-60 seconds total

# DON'T do this before tests:
# make clean-tests  # This would delete cache!

# If you need to clean a specific test:
rm -rf ../output/TestProject/bin
rm -rf ../output/TestProject/CMakeFiles
# But keep ../output/TestProject/build/_deps/

# Full clean only when necessary:
make clean-all  # Removes everything including cache
```

### When cache is automatically invalidated:
- Changes to CMakeLists.txt dependencies
- Changes to compiler flags
- Corrupted cache files

### Debugging slow tests:
```bash
# Check if cache exists
ls -la ../output/YourTest/build/_deps/

# Run single test with verbose output
python3 ../tests/test_fast_build.py -v

# Force full rebuild for one project
rm -rf ../output/YourTest/build/_deps/
```

## Common CLI Usage

```bash
# Interactive console (F1 in-game or ` key)
./game

# Single command
./game --command "project.list"
./game --json -c "entity.list"

# Batch commands
./game --batch "project.create MyGame" "scene.create main" "project.build"

# Script execution
./game --script build_commands.txt
./game --headless --script ../tests/test_script.txt

# Project workflow
./game -c "project.create MyGame"
./game -c "project.open MyGame"
./game -c "scene.create main"
./game -c "entity.create Player"
./game -c "project.build"     # Creates release build
./game -c "project.run"       # Runs built executable
```

## Architecture Overview

This is an Entity Component System (ECS) game engine using EnTT with a comprehensive CLI and build system.

### Core Architecture

- **Engine** (src/engine.h): Main class managing initialization, game loop (60 FPS), and shutdown
- **EngineCore** (src/engine/engine_core.h): Core functionality shared between regular and CLI modes
- **Scene** (src/scene/scene.h): Base class with EnTT registry, lifecycle methods (onCreate, onUpdate, onDestroy)
- **SystemsManager**: Manages all ECS systems with proper initialization order

### CLI System

- **CLIEngine**: Wrapper around Engine for command-line operations
- **CLIArgumentParser**: Parses command-line arguments
- **CommandRegistry**: Extensible command system with categories (project, scene, entity, build)
- **CLIResult**: Structured results with JSON serialization support

### Build System

- **Async builds**: Non-blocking build execution
- **Template system**: Uses CMakeLists_template.txt and game_template.cpp
- **Game runtime**: Minimal executable that loads and runs built games
- **Asset packaging**: Automatic copying of assets, scenes, and configuration

### Component Model

- **TransformComponent**: 3D position, rotation, scale using RayLib Vector3
- **Sprite**: 2D rendering with Texture2D, source rectangle, tint color
- Components are registered in ComponentRegistry for serialization

### Play Mode

- **F5**: Start/Stop play mode
- **F6**: Pause/Resume
- Console commands: `play`, `stop`, `pause`, `resume`
- Hot-reload support for Lua scripts

### Resource Management

- Thread-safe ResourceManager with reference counting
- Supports textures, sounds, fonts, models
- Silent operation in headless mode
- Automatic cleanup on shutdown

### Project Structure

Projects contain:
- `project.json`: Project metadata and configuration
- `scenes/`: Scene JSON files
- `assets/`: Textures, sounds, fonts
- `scripts/`: Lua scripts
- `build/`: Build output directory

### Key Integration Points

- **RayLib 5.0**: Windowing, rendering, input
- **EnTT 3.13.2**: ECS framework
- **spdlog 1.14.1**: Logging (debug level by default)
- **nlohmann/json 3.11.3**: JSON parsing for CLI and serialization
- **Lua**: Scripting with bindings for engine functionality
- **GLM 1.0.1**: Math library with RayLib conversions

### Testing Strategy

1. **Python tests**: CLI functionality, project management, scene operations
2. **Script tests**: Batch command execution via .txt files
3. **C++ tests**: ResourceManager, memory safety, threading
4. **JSON validation**: All commands support --json for structured output

When implementing features:
- Follow ECS pattern: data in components, logic in systems
- Add CLI commands to appropriate category in CommandRegistry
- Support both interactive and headless modes
- Include JSON output for automation
- Write tests for new functionality