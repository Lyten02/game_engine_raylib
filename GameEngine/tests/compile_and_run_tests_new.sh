#!/bin/bash

# New version using enhanced C++ test runner

echo "🧪 GameEngine C++ Test Suite"
echo "================================================"
echo ""

# Change to tests directory
cd "$(dirname "$0")"

# Run the enhanced test suite with all passed arguments
./run_cpp_test_suite.sh "$@"