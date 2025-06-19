#include <iostream>
#include "../src/packages/package_manager.h"

// Test helper macros
#define TEST_ASSERT(condition, message) \
    if (!(condition)) { \
        std::cerr << "FAIL: " << message << std::endl; \
        return 1; \
    }

#define TEST_VERSION(required, actual, expected) \
    { \
        bool result = manager.isVersionCompatible(required, actual); \
        if (result != expected) { \
            std::cerr << "FAIL: Version check '" << required << "' vs '" << actual \
                     << "' expected " << (expected ? "compatible" : "incompatible") \
                     << " but got " << (result ? "compatible" : "incompatible") << std::endl; \
            return 1; \
        } \
    }

int main() {
    std::cout << "Running version validation tests..." << std::endl;
    
    PackageManager manager("/tmp/test");
    manager.setEngineVersion("0.2.0");
    
    // Test 1: Basic version comparisons
    {
        std::cout << "\nTest 1: Basic version comparisons..." << std::endl;
        
        // Greater than or equal
        TEST_VERSION(">=1.0.0", "1.0.0", true);
        TEST_VERSION(">=1.0.0", "1.0.1", true);
        TEST_VERSION(">=1.0.0", "2.0.0", true);
        TEST_VERSION(">=1.0.0", "0.9.9", false);
        
        // Greater than
        TEST_VERSION(">1.0.0", "1.0.1", true);
        TEST_VERSION(">1.0.0", "2.0.0", true);
        TEST_VERSION(">1.0.0", "1.0.0", false);
        TEST_VERSION(">1.0.0", "0.9.9", false);
        
        // Less than or equal
        TEST_VERSION("<=1.0.0", "1.0.0", true);
        TEST_VERSION("<=1.0.0", "0.9.9", true);
        TEST_VERSION("<=1.0.0", "1.0.1", false);
        
        // Less than
        TEST_VERSION("<1.0.0", "0.9.9", true);
        TEST_VERSION("<1.0.0", "1.0.0", false);
        TEST_VERSION("<1.0.0", "1.0.1", false);
        
        // Exact match
        TEST_VERSION("==1.0.0", "1.0.0", true);
        TEST_VERSION("==1.0.0", "1.0.1", false);
        TEST_VERSION("=1.0.0", "1.0.0", true);
        
        std::cout << "PASS: Basic version comparisons" << std::endl;
    }
    
    // Test 2: Caret ranges
    {
        std::cout << "\nTest 2: Caret version ranges..." << std::endl;
        
        // ^1.2.3 means >=1.2.3 and <2.0.0
        TEST_VERSION("^1.2.3", "1.2.3", true);
        TEST_VERSION("^1.2.3", "1.2.4", true);
        TEST_VERSION("^1.2.3", "1.3.0", true);
        TEST_VERSION("^1.2.3", "1.9.9", true);
        TEST_VERSION("^1.2.3", "2.0.0", false);
        TEST_VERSION("^1.2.3", "1.2.2", false);
        
        // ^0.x.y is special - only patch updates allowed
        TEST_VERSION("^0.1.0", "0.1.0", true);
        TEST_VERSION("^0.1.0", "0.1.1", true);
        TEST_VERSION("^0.1.0", "0.2.0", false);
        TEST_VERSION("^0.1.0", "1.0.0", false);
        
        std::cout << "PASS: Caret version ranges" << std::endl;
    }
    
    // Test 3: Multi-part versions
    {
        std::cout << "\nTest 3: Multi-part version numbers..." << std::endl;
        
        TEST_VERSION(">=1.2.3", "1.2.3", true);
        TEST_VERSION(">=1.2.3", "1.2.4", true);
        TEST_VERSION(">=1.2.3", "1.3.0", true);
        TEST_VERSION(">=1.2.3", "2.0.0", true);
        TEST_VERSION(">=1.2.3", "1.2.2", false);
        TEST_VERSION(">=1.2.3", "1.1.9", false);
        
        std::cout << "PASS: Multi-part version numbers" << std::endl;
    }
    
    // Test 4: Engine compatibility
    {
        std::cout << "\nTest 4: Engine compatibility checks..." << std::endl;
        
        Package pkg1("test1", "1.0.0");
        pkg1.setEngineVersion(">=0.1.0");
        TEST_ASSERT(manager.checkEngineCompatibility(pkg1), "Package requiring >=0.1.0 should be compatible with engine 0.2.0");
        
        Package pkg2("test2", "1.0.0");
        pkg2.setEngineVersion(">=0.3.0");
        TEST_ASSERT(!manager.checkEngineCompatibility(pkg2), "Package requiring >=0.3.0 should not be compatible with engine 0.2.0");
        
        Package pkg3("test3", "1.0.0");
        pkg3.setEngineVersion("^0.2.0");
        TEST_ASSERT(manager.checkEngineCompatibility(pkg3), "Package requiring ^0.2.0 should be compatible with engine 0.2.0");
        
        std::cout << "PASS: Engine compatibility checks" << std::endl;
    }
    
    // Test 5: No version requirement
    {
        std::cout << "\nTest 5: No version requirement..." << std::endl;
        
        TEST_VERSION("", "1.0.0", true);
        TEST_VERSION("", "0.0.1", true);
        TEST_VERSION("", "999.999.999", true);
        
        Package pkg("test", "1.0.0");
        pkg.setEngineVersion("");
        TEST_ASSERT(manager.checkEngineCompatibility(pkg), "Package with no engine version should always be compatible");
        
        std::cout << "PASS: No version requirement" << std::endl;
    }
    
    std::cout << "\nAll version validation tests passed!" << std::endl;
    return 0;
}