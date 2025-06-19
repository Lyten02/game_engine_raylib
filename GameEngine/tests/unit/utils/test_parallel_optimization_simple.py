#!/usr/bin/env python3
"""Simple test to verify parallel optimization settings"""

import sys
import os

# Test imports and configuration
try:
    from test_categories import TestCategorizer
    tc = TestCategorizer()
    
    print("=== PARALLEL OPTIMIZATION CONFIGURATION ===")
    print(f"Worker limits configured: {tc.category_limits}")
    
    # Verify conservative limits
    all_limited = all(limit <= 2 for limit in tc.category_limits.values())
    
    if all_limited:
        print("✅ All categories have conservative worker limits (≤2)")
    else:
        print("❌ Some categories have excessive worker limits")
        sys.exit(1)
    
    # Test resource limits file
    if os.path.exists("resource_limits.json"):
        print("✅ Resource limits configuration file exists")
    else:
        print("⚠️  Resource limits file not found")
    
    print("\n✅ Parallel optimization configured correctly!")
    sys.exit(0)
    
except Exception as e:
    print(f"❌ Configuration error: {e}")
    sys.exit(1)