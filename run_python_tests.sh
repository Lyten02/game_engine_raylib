#!/bin/zsh
set -e

echo "🧪 GameEngine Python Test Suite"
echo "================================"
echo "Working directory: $(pwd)"
echo "Project directory: $PROJECT_DIR"

# Change to GameEngine build directory  
cd "GameEngine/build"
echo "Changed to: $(pwd)"

# Check if game_engine exists
if [ ! -f game_engine ]; then
    echo "⚠️  game_engine executable not found!"
    echo "🔨 Building game_engine first..."
    make -j8
    if [ $? -ne 0 ]; then
        echo "❌ Build failed!"
        exit 1
    fi
fi

echo "✅ game_engine found, size: $(ls -lh game_engine | awk '{print $5}')"

# Set Python path and run tests
export PYTHONPATH="../tests:$PYTHONPATH"
echo "🐍 Python version: $(/Users/konstantin/.pyenv/versions/3.13.1/bin/python3 --version)"
echo "📂 PYTHONPATH: $PYTHONPATH"
echo ""
echo "🚀 Starting test execution..."
echo "================================"

# Execute the tests
/Users/konstantin/.pyenv/versions/3.13.1/bin/python3 ../tests/run_all_tests.py

echo ""
echo "✅ Python tests completed!"