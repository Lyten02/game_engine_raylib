# Log Control Documentation

This document describes how to control logging in the GameEngine, particularly useful for testing and automation.

## Command Line Options

### Log Level Control

You can control the verbosity of logs using the following options:

- `--log-level <level>` - Set specific log level
  - `trace` - Most verbose, includes trace messages
  - `debug` - Debug messages and above
  - `info` - Informational messages and above (default)
  - `warn` - Only warnings and errors
  - `error` - Only error messages
  - `off` - Disable all logging

- `-q, --quiet` - Suppress non-critical logs (equivalent to `--log-level warn`)
- `--verbose` - Enable verbose output (equivalent to `--log-level debug`)
- `--json` - When using JSON output, all logs are automatically suppressed

### Examples

```bash
# Run with only error logs
./game --headless --command "project.build" --log-level error

# Run in quiet mode (only warnings and errors)
./game --headless --batch "project.create test" "project.build" --quiet

# Run with debug logging
./game --headless --script test.txt --verbose

# Disable all logs
./game --headless --command "project.list" --log-level off
```

## Log Limiting

The engine includes automatic log limiting to prevent spam when running repetitive operations (common in tests).

### How It Works

- In batch mode or single command mode, repetitive log messages are automatically limited
- By default, the same message will only be logged 3 times per minute
- After the limit is reached, a debug message indicates the suppression

### Affected Messages

The following messages are subject to limiting:
- "Project loaded: <name>" - When loading many projects
- "Build already in progress!" - When attempting concurrent builds
- "Building project: <name>" - In test mode builds

### Configuration

Log limiting is automatically enabled for:
- Batch mode (`--batch`)
- Single command mode (`--command`)
- Script execution (`--script`)

It is not applied in interactive mode to maintain full visibility during development.

## Best Practices for Tests

When writing tests that create multiple projects or run repetitive commands:

1. **Use `--quiet` for cleaner test output**:
   ```bash
   ./game --headless --batch "project.create test1" "project.create test2" --quiet
   ```

2. **Use `--log-level error` to only see failures**:
   ```bash
   ./game --headless --script bulk_test.txt --log-level error
   ```

3. **Use JSON output for parsing results**:
   ```bash
   ./game --json --headless --command "project.list"
   ```

4. **For debugging failing tests, use `--verbose`**:
   ```bash
   ./game --headless --command "project.build" --verbose
   ```

## Implementation Details

The log limiting system uses the `LogLimiter` utility class which:
- Tracks message frequency by key
- Limits messages to a configurable maximum per time window
- Can be disabled globally if needed
- Provides statistics about suppressed messages

For developers extending the engine, you can use `LogLimiter` for any repetitive logs:

```cpp
#include "utils/log_limiter.h"

// Instead of:
spdlog::info("Processing item: {}", itemName);

// Use:
LogLimiter::info("processing_item", "Processing item: {}", itemName);
```

### C++ Test Log Control

When writing C++ tests that may produce repetitive logs:

1. **Set appropriate log level in test main**:
   ```cpp
   int main() {
       spdlog::set_level(spdlog::level::warn);  // Only warnings and errors
       // Your test code...
   }
   ```

2. **The LogLimiter is automatically active** and limits repetitive messages to:
   - ResourceManager warnings (texture not found, etc.)
   - Build system messages (build already in progress)
   - Project loading messages

3. **Running C++ tests with reduced logging**:
   ```bash
   # The compile_and_run_tests.sh script runs tests normally
   # Log limiting is built into the engine components
   make test-cpp
   ```