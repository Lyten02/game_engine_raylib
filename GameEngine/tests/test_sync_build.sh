#!/bin/bash
# Quick test of the synchronous build command

echo "Testing synchronous build command..."

cd ../build

# Clean up any existing test project
rm -rf projects/BuildTest output/BuildTest

# Run the test
python3 ../tests/test_build_system.py

echo "Done!"