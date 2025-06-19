#!/bin/bash
# Safe test runner with resource limits

echo "🛡️ Safe Test Runner - Resource Limited"
echo "======================================"

# Kill any existing processes first
./emergency_stop.sh

# Set resource limits
ulimit -v 2097152  # 2GB virtual memory limit
ulimit -t 300      # 5 minute CPU time limit

# Run tests with strict limits
echo "Running tests with resource limits..."
echo "- Memory limit: 2GB"
echo "- CPU time limit: 5 minutes"
echo "- Sequential execution only"

# Run only lightweight tests first
python3 run_all_tests.py \
    --skip-full-build \
    --memory-monitor \
    --memory-limit 1500 \
    test_cli_basic.py \
    test_config_system.py \
    test_logging.py

echo "✅ Safe test run completed"