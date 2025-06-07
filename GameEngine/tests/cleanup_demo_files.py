#!/usr/bin/env python3
"""Clean up demo files"""

import os
import glob

files_to_remove = []

# Demo files
files_to_remove.extend(glob.glob("demo_*.py"))
files_to_remove.extend(glob.glob("test_enhanced_runner_demo.py"))

# Log files
files_to_remove.extend(glob.glob("test_log_*.log"))

# Results file
files_to_remove.extend(glob.glob("test_results.json"))

# Remove files
for file in files_to_remove:
    if os.path.exists(file):
        os.remove(file)
        print(f"Removed: {file}")

print(f"\n✅ Cleaned up {len(files_to_remove)} files")