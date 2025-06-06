# CLI System Documentation

## Overview

The GameEngine now supports a comprehensive CLI (Command Line Interface) system that enables:
- Automated testing
- Headless operation (no graphics window)
- Batch command execution
- JSON output for programmatic integration
- Script file execution

## Basic Usage

### Interactive Mode (Default)
```bash
./game
```
Launches the game engine with the graphics window as before.

### CLI Commands

#### Help and Version
```bash
./game --help          # Show help message
./game --version       # Show version information
```

#### Single Command Execution
```bash
./game --command "project.list"
./game --json --command "help"              # JSON output
./game --headless -c "project.create test"  # No graphics window
```

#### Batch Commands
```bash
./game --batch "project.create test" "entity.create Player" "entity.list"
```

#### Script Execution
```bash
./game --script tests/basic_test.txt
./game --json --script tests/regression.txt
```

#### Project Operations
```bash
./game --project "MyGame" --command "scene.list"
./game --headless --project "MyGame" --batch "entity.create Enemy" "component.add Transform 100 100 0"
```

## JSON Output Format

When using `--json` flag, all output is in structured JSON:

```json
{
  "success": true,
  "output": "Command output text",
  "error": "",
  "exit_code": 0,
  "data": {
    // Optional structured data
  }
}
```

## Script File Format

Script files are plain text with one command per line:
```bash
# Comments start with #
project.create test_project
project.open test_project

# Create scene
scene.create main_scene

# Create entities
entity.create Player
component.add Transform 0 0 0
component.add Sprite player.png

# Save and close
scene.save main_scene
project.close
```

## Exit Codes

- `0` - Success
- `1` - Command error
- `2` - Invalid arguments
- `3` - Engine initialization failed
- `4` - Project operation failed

## Python Integration Example

```python
import subprocess
import json

def run_engine_command(command):
    result = subprocess.run(
        ["./game", "--json", "--headless", "--command", command],
        capture_output=True,
        text=True
    )
    return json.loads(result.stdout)

# Test project creation
result = run_engine_command("project.create test")
assert result["success"] == True
```

## Implementation Details

### Architecture

1. **CLIArgumentParser** - Parses command line arguments
2. **CLIEngine** - Wrapper around Engine for CLI operations
3. **CLIResult** - Structured result format with JSON serialization
4. **Headless Mode** - Engine runs without Raylib window

### Key Features

- **Silent Mode**: In headless mode, logging is suppressed to ensure clean JSON output
- **Batch Execution**: Multiple commands can be executed in sequence
- **Error Handling**: Failed commands return structured error information
- **Resource Management**: ResourceManager operates in silent mode during headless operation

### File Structure
```
src/cli/
├── cli_argument_parser.h/.cpp  # Command line parsing
├── cli_engine.h/.cpp           # CLI engine wrapper
└── cli_result.h                # Result structures
```

## Testing

Run the basic test suite:
```bash
python3 tests/test_cli_basic.py
```

Run a test script:
```bash
./game --json --script tests/basic_cli_test.txt
```

## Future Enhancements

- [ ] Capture actual command output (not just "Command executed")
- [ ] Add test framework integration
- [ ] Support for parallel command execution
- [ ] Command result validation
- [ ] Performance benchmarking support

## Benefits

1. **Automated Testing**: Claude Coder and CI/CD can test features automatically
2. **Regression Prevention**: Easy to verify nothing breaks with new changes
3. **Documentation Testing**: Examples in docs can be validated
4. **Development Speed**: No manual testing required for basic operations
5. **Integration Ready**: JSON output makes it easy to integrate with other tools