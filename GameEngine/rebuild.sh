#!/bin/bash

echo "Cleaning build directory..."
rm -rf build
mkdir build
cd build

echo "Running CMake..."
cmake ..

echo "Building project..."
make -j8

echo "Build complete!"