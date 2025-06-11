# Build System Documentation

## Overview

The GameEngine build system allows you to compile your projects into standalone executable files. It uses CMake under the hood to manage dependencies and compilation.

## Features

- **Automatic Dependency Management**: All required libraries (RayLib, EnTT, spdlog, etc.) are automatically downloaded and linked
- **Cross-Platform Support**: Builds work on Windows, Linux, and macOS
- **Multiple Build Configurations**: Support for Release and Debug builds
- **Asset Packaging**: Automatically copies assets and scenes to the output directory

## How to Build a Project

### 1. Using the Console (Interactive Mode)

```bash
# Start the engine
./game

# Press F1 to open console
# Create and build a project
project.create MyGame
project.open MyGame
scene.create main
entity.create Player
scene.save main
project.build
```

### 2. Using Command Line (Batch Mode)

Create a script file `build_script.txt`:
```
project.create MyGame
project.open MyGame
scene.create main
entity.create Player
scene.save main
project.build
```

Run it:
```bash
./game --headless --script build_script.txt
```

### Available Build Commands

- `project.build` - Full build from scratch (downloads dependencies if needed, ~60s first time)
- `project.build.fast` - Fast build using cached dependencies (~3-6s)
- `project.prepare` - Prepare project files without compilation (instant, for editor preview)
- `project.clean` - Clean build artifacts
- `project.rebuild` - Clean and build from scratch

## Build Output Structure

After a successful build, your project will have the following structure:

```
output/
└── MyGame/
    ├── bin/
    │   ├── MyGame           # Executable (Linux/Mac)
    │   ├── MyGame.exe       # Executable (Windows)
    │   ├── assets/          # Copied assets
    │   ├── scenes/          # Scene JSON files
    │   └── game_config.json # Game configuration
    ├── build/               # CMake build files
    └── main.cpp            # Generated source code
```

## Build Process

1. **Directory Creation**: Creates output directory structure
2. **Code Generation**: Generates main.cpp from template with game runtime
3. **CMake Configuration**: Creates CMakeLists.txt with all dependencies
4. **Compilation**: Runs CMake to compile the executable
5. **Asset Packaging**: Copies assets, scenes, and configuration

## Build Configurations

- **Release** (default): Optimized for performance
- **Debug**: Includes debug symbols for development

```bash
project.build         # Release build
project.build Debug   # Debug build
```

## Running Built Games

After building, you can run your game:

```bash
# From console
project.run

# From command line
./output/MyGame/bin/MyGame
```

## Troubleshooting

### Build Takes Too Long
The first build downloads all dependencies, which can take several minutes. Subsequent builds are faster.

### Build Fails
1. Check that CMake 3.20+ is installed
2. Check that a C++20 compiler is available
3. Check console output for specific error messages

### Game Doesn't Run
1. Ensure all assets are in the project's assets/ directory
2. Check that scenes are saved before building
3. Verify game_config.json exists in the output directory

## Technical Details

### Dependencies
The build system uses CMake's FetchContent to automatically download:
- RayLib 5.0 (graphics/window)
- EnTT 3.13.2 (ECS framework)
- spdlog 1.14.1 (logging)
- GLM 1.0.1 (math)
- nlohmann/json 3.11.3 (JSON parsing)

### Templates
- **CMakeLists_template.txt**: Template for CMake configuration
- **game_template.cpp**: Template for the game executable
- **Game Runtime**: Minimal runtime that loads and runs scenes

### Async Build System
Builds run asynchronously to avoid blocking the engine. Progress updates appear in the console.

## Make Targets

The build system provides several useful make targets:

### Running Tests
```bash
make test         # Run all tests (Python and scripts)
make test-cpp     # Compile and run C++ tests
```

### Cleaning
```bash
make clean        # Standard CMake clean (removes build artifacts)
make clean-tests  # Clean test projects and executables
make clean-logs   # Remove log files
make clean-all    # Full clean (includes all of the above)
```

### Examples
```bash
# Build the engine
cd GameEngine/build
cmake ..
make

# Run tests
make test

# Clean everything and rebuild
make clean-all
cmake ..
make
```