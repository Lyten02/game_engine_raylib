#!/usr/bin/env python3
"""Verify test directory structure"""

import os
import sys

def verify_structure():
    """Check if all required directories exist"""
    base_path = os.path.dirname(os.path.abspath(__file__))
    
    required_dirs = [
        # Unit tests
        "unit",
        "unit/ecs",
        "unit/resources",
        "unit/utils",
        "unit/serialization",
        
        # Integration tests
        "integration",
        "integration/cli",
        "integration/packages",
        "integration/projects",
        "integration/build",
        
        # System tests
        "system",
        "system/performance",
        "system/security",
        "system/platform",
        "system/e2e",
        
        # Support directories
        "fixtures",
        "fixtures/projects",
        "fixtures/assets",
        "fixtures/configs",
        
        "utils",
        "tools"
    ]
    
    missing = []
    for dir_path in required_dirs:
        full_path = os.path.join(base_path, dir_path)
        if not os.path.exists(full_path):
            missing.append(dir_path)
            print(f"❌ Missing: {dir_path}")
        else:
            # Check for README.md
            readme_path = os.path.join(full_path, "README.md")
            if not os.path.exists(readme_path):
                print(f"⚠️  No README.md in: {dir_path}")
    
    if missing:
        print(f"\n❌ {len(missing)} directories missing")
        return False
    else:
        print("\n✅ All directories exist")
        return True

if __name__ == "__main__":
    if not verify_structure():
        sys.exit(1)