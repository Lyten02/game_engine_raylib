#!/usr/bin/env python3
"""Check template content"""

import os

template_path = "../templates/basic/game_template.cpp"
if not os.path.exists(template_path):
    template_path = "templates/basic/game_template.cpp"

print(f"Reading template from: {os.path.abspath(template_path)}")

with open(template_path, 'r') as f:
    content = f.read()

# Find the initialize line
lines = content.split('\n')
for i, line in enumerate(lines):
    if 'runtime.initialize' in line:
        print(f"Line {i+1}: {line.strip()}")
        
# Check for any PROJECT_NAME replacements
if "{{PROJECT_NAME}}" in content:
    print("\nFound {{PROJECT_NAME}} placeholders:")
    for i, line in enumerate(lines):
        if "{{PROJECT_NAME}}" in line:
            print(f"Line {i+1}: {line.strip()}")