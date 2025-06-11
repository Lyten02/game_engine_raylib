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

## Test Suite Logging System

The GameEngine test suite includes an enhanced logging system that provides detailed information about test execution, making it easier to debug failures and track performance.

### Log File Structure

When running tests, a timestamped log file is created (e.g., `test_log_20250611_220519.log`) with the following structure:

1. **Header Section**
   - Start time
   - Python version
   - Platform information
   - Working directory
   - Test configuration (skip full build, verbose mode, parallel mode)

2. **Test Execution Details**
   - Each test is clearly separated with dividers
   - Test start/end timestamps
   - Test type (Python, Script, Command)
   - Full file paths
   - Execution duration
   - Return codes

3. **Error Reporting**
   - Complete stdout/stderr output for failed tests
   - Full stack traces for exceptions
   - Command line used to run the test
   - Timeout information if applicable

4. **Summary Sections**
   - Execution summary with pass/fail counts
   - Test results grouped by type
   - Failed tests summary with error previews
   - Test execution time breakdown table

### Running Tests with Enhanced Logging

```bash
# Standard test run (creates log file automatically)
python3 tests/run_all_tests.py

# Verbose mode - shows real-time error details
python3 tests/run_all_tests.py --verbose

# Parallel mode with enhanced logging
python3 tests/run_all_tests.py --parallel

# Skip full build tests (faster)
python3 tests/run_all_tests.py --skip-full-build
```

### Log File Examples

**Successful Test Entry:**
```
[2025-06-11 22:13:59.058] [INFO    ] ============================================================
[2025-06-11 22:13:59.058] [INFO    ] TEST START: test_categories.py (1/43)
[2025-06-11 22:13:59.058] [INFO    ] Type: Python Test
[2025-06-11 22:13:59.058] [INFO    ] File: ../tests/test_categories.py
[2025-06-11 22:13:59.058] [INFO    ] ============================================================
[2025-06-11 22:13:59.083] [SUCCESS ] TEST PASSED: test_categories.py
[2025-06-11 22:13:59.083] [INFO    ] Duration: 0.02 seconds
[2025-06-11 22:13:59.083] [INFO    ] Return Code: 0
```

**Failed Test Entry:**
```
[2025-06-11 22:14:26.310] [ERROR   ] TEST FAILED: test_example.py
[2025-06-11 22:14:26.310] [ERROR   ] Duration: 0.02 seconds
[2025-06-11 22:14:26.310] [ERROR   ] Return Code: 1
[2025-06-11 22:14:26.311] [ERROR   ] Command: python3 ../tests/test_example.py
[2025-06-11 22:14:26.311] [ERROR   ] ======================================== STDOUT ========================================
[2025-06-11 22:14:26.311] [ERROR   ] Debug output before error...
[2025-06-11 22:14:26.311] [ERROR   ] ======================================== STDERR ========================================
[2025-06-11 22:14:26.311] [ERROR   ] Traceback (most recent call last):
  File "test_example.py", line 12, in test_function
    assert False, "Test assertion failed"
AssertionError: Test assertion failed
```

### Parallel Test Logging

When using `--parallel` mode, the log file includes:
- Worker ID for each test
- Test results grouped by type (Python, Script, Command)
- Performance metrics for parallel execution
- Clear indication of which worker processed each test

### Log File Locations

- **Log Files**: `test_log_YYYYMMDD_HHMMSS.log` in the current directory
- **JSON Results**: `test_results.json` with structured test data
- **Parallel Results**: `parallel_test_results.json` (when using --parallel)

### Best Practices

1. **Always check the log file for failed tests** - it contains complete error information
2. **Use --verbose during development** to see errors in real-time
3. **Archive log files from CI/CD** for debugging intermittent failures
4. **The log file timestamp matches test start time** for easy correlation

### Troubleshooting with Logs

1. **Finding specific test failures:**
   ```bash
   grep "TEST FAILED" test_log_*.log
   ```

2. **Viewing full error for a specific test:**
   ```bash
   grep -A 50 "TEST FAILED: test_name.py" test_log_*.log
   ```

3. **Checking test execution times:**
   ```bash
   grep -E "Duration: [0-9.]+ seconds" test_log_*.log | sort -k3 -n
   ```