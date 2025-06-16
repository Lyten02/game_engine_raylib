#!/usr/bin/env python3
"""Check working directory and paths"""

import os
import subprocess

# Create a simple command to check paths
with open("debug_paths.txt", "w") as f:
    f.write("help\n")  # Just a simple command to run

# Run and capture working directory
result = subprocess.run(
    ["../build/game_engine", "--headless", "--script", "debug_paths.txt"],
    capture_output=True,
    text=True,
    cwd="."
)

print("Current test directory:", os.getcwd())
print("\nChecking if templates exist from test directory:")
print("templates/basic/game_template.cpp:", os.path.exists("templates/basic/game_template.cpp"))
print("../templates/basic/game_template.cpp:", os.path.exists("../templates/basic/game_template.cpp"))

# Check from build directory
os.chdir("../build")
print("\nCurrent directory after change:", os.getcwd())
print("Checking if templates exist from build directory:")
print("templates/basic/game_template.cpp:", os.path.exists("templates/basic/game_template.cpp"))
print("../templates/basic/game_template.cpp:", os.path.exists("../templates/basic/game_template.cpp"))

# Clean up
os.chdir("../tests")
os.remove("debug_paths.txt")