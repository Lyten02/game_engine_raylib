#!/bin/bash

echo "Testing Default Texture System..."
cd /Users/konstantin/Desktop/Code/GameEngineRayLib/GameEngine

# Create a test project without textures
echo "Creating test project..."
echo -e "project.create TestMissing\nproject.open TestMissing\nexit\n" | ./build/game --cli > /dev/null 2>&1

# Add entity with missing texture
echo "Adding entity with missing texture..."
echo -e "project.open TestMissing\nentity.create Player\ncomponent.add Sprite\ncomponent.set Sprite texture \"nonexistent.png\"\nscene.save main\nexit\n" | ./build/game --cli

echo ""
echo "Building and running the test project..."
cd projects/TestMissing
mkdir -p build
cd build
cmake ..
make

echo ""
echo "The game should now run with default textures (pink-black checkerboard) for missing textures."
echo "Running the game for 5 seconds..."
timeout 5 ./TestMissing || true

echo ""
echo "Test completed. Check if entities are visible with default textures."