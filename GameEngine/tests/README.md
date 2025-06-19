# GameEngine Tests

## 🚀 Quick Start

```bash
cd GameEngine/build
make test          # Run all tests
make test-fast     # Run tests without rebuild
make test-cpp      # Run C++ tests (when available)
make clean-tests   # Clean test output
```

## 📁 Test Organization

```
tests/
├── unit/                    # Unit tests
│   ├── ecs/                # ECS components and systems
│   ├── resources/          # Resource management
│   ├── utils/              # Utilities and helpers
│   └── serialization/      # Serialization tests
│
├── integration/            # Integration tests
│   ├── cli/               # CLI interaction
│   ├── packages/          # Package system
│   ├── projects/          # Project management
│   └── build/             # Build system
│
├── system/                # System tests
│   ├── performance/       # Performance benchmarks
│   ├── security/          # Security tests
│   ├── platform/          # Cross-platform tests
│   └── e2e/              # End-to-end scenarios
│
├── fixtures/              # Test data and resources
├── utils/                 # Test utilities
└── tools/                 # Test tools
```

## 🛠️ Available Make Targets

- `make test` - Run all tests with full build
- `make test-fast` - Run tests without rebuilding
- `make test-cpp` - Run C++ unit tests
- `make clean-tests` - Clean test output directory
- `make clean-logs` - Remove log files
- `make clean-all` - Full clean including dependencies

## 📝 Test Types

### Python Tests (.py)
Located in `unit/` and `integration/` directories

### C++ Tests (.cpp)
Located in `unit/` and `integration/` directories

### Script Tests (.txt)
Located in `integration/cli/` directory

## 🔧 Running Individual Tests

### Python Test
```bash
cd build
python3 ../tests/unit/utils/test_path_resolver.py
```

### Script Test
```bash
cd build
./game_engine --script ../tests/integration/cli/basic_cli_test.txt
```

## 📊 Test Results

Test results are saved to:
- Console output with progress bar
- `test_results.json` - Structured results
- `logs/test_log_*.log` - Detailed execution logs