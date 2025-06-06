#!/bin/bash
# Debug script to test synchronous build

echo "=== Debug Build Test ==="
echo "Creating batch file..."

cat > debug_build_commands.txt << EOF
project.create DebugBuildTest
project.open DebugBuildTest
scene.create main
entity.create Player
scene.save main
project.build.sync
EOF

echo "Running commands..."
../build/game --headless --script debug_build_commands.txt

echo ""
echo "Checking output directory..."
ls -la output/DebugBuildTest/bin/ 2>/dev/null || echo "Bin directory not found"

echo ""
echo "Checking project structure..."
find projects/DebugBuildTest -type f 2>/dev/null | head -20

echo ""
echo "Checking build directory..."
ls -la output/DebugBuildTest/build/ 2>/dev/null || echo "Build directory not found"

# Cleanup
rm -rf projects/DebugBuildTest output/DebugBuildTest debug_build_commands.txt

echo "Done!"