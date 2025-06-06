# CLI Commands Reference

## Basic CLI Usage

```bash
# Show help
./game --help

# Show version
./game --version

# Run in headless mode (no graphics window)
./game --headless

# Execute single command
./game --command "help"
./game -c "project.list"

# Execute with JSON output
./game --json --command "help"

# Execute batch commands
./game --batch "project.create test" "entity.create Player"

# Execute from script file
./game --script commands.txt

# Open project and execute command
./game --project myproject --command "scene.list"
```

## Available Commands (from code analysis)

### General Commands
- `help [command]` - Show help for all commands or specific command
- `clear` - Clear console output
- `quit` / `exit` - Exit the application

### Project Management
- `project.create <name>` - Create a new project
- `project.open <name>` - Open an existing project
- `project.close` - Close current project
- `project.list` - List all projects
- `project.current` - Show current project info
- `project.save` - Save current project
- `project.refresh` - Refresh project view

### Scene Management
- `scene.create <name>` - Create a new scene
- `scene.save [name]` - Save current scene
- `scene.load <name>` - Load a scene
- `scene.list` - List all scenes
- `scene.clear` - Clear current scene

### Entity Management
- `entity.create` - Create a new test entity
- `entity.destroy <id>` - Destroy entity by ID
- `entity.list` - List all entities with components

### Engine Commands
- `engine.info` - Display engine information
- `engine.fps <limit>` - Set FPS limit (0 for unlimited)
- `engine.vsync` - Toggle V-Sync
- `engine.diag` - Show performance diagnostics

### Build System
- `build.play` - Build current project in play mode
- `build.editor` - Build for editor
- `build.standalone` - Build standalone executable
- `build.package <type>` - Build release package

### Debug Commands
- `debug.toggle` - Toggle debug info display
- `debug.log <level>` - Set log level (trace/debug/info/warn/error/critical/off)

### Console Commands
- `console.fps` - Toggle FPS display in console

### Configuration
- `config.get <key>` - Get configuration value
- `config.set <key> <value>` - Set configuration value (runtime only)
- `config.reload` - Reload config from file

### Script Commands
- `script.execute <path>` - Execute Lua script
- `script.reload <path>` - Reload and execute script
- `script.list` - List loaded scripts
- `script.eval <code>` - Execute Lua code directly

### Render Commands
- `render.stats` - Display render statistics

### Log Commands
- `logs.open` - Open logs folder
- `logs.list` - List log files

### Package Commands (if enabled)
- `package.search <query>` - Search for packages
- `package.install <name>` - Install a package
- `package.remove <name>` - Remove a package
- `package.list` - List installed packages
- `package.update [name]` - Update packages
- `package.info <name>` - Show package details

## JSON Output Format

When using `--json` flag, all commands return JSON in this format:

```json
{
  "success": true,
  "output": "Command output text",
  "error": "",
  "exitCode": 0,
  "data": {
    // Optional structured data
  }
}
```

## Example Automation Script

```python
import subprocess
import json

def run_game_command(command, headless=True):
    """Execute a game engine CLI command and return JSON result"""
    args = ["./game", "--json"]
    if headless:
        args.append("--headless")
    args.extend(["--command", command])
    
    result = subprocess.run(args, capture_output=True, text=True)
    return json.loads(result.stdout)

# Create a project
result = run_game_command("project.create my_game")
if result["success"]:
    print("Project created successfully!")

# Create some entities
for i in range(5):
    result = run_game_command("entity.create")
    print(f"Created entity: {result['output']}")

# List all entities
result = run_game_command("entity.list")
print(result["output"])
```

## Testing the CLI

Run the Python test script to verify all CLI functionality:

```bash
cd tests
python3 test_cli_json.py
```

This will test:
- Basic help and version commands
- JSON output formatting
- Headless mode operation
- Project creation
- Batch command execution
- Error handling