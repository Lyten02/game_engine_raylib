#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include "../src/utils/config.h"
#include <nlohmann/json.hpp>

// Simple test framework
bool testsPassed = true;
bool testsInitialized = false;

#define TEST(name) void name(); \
    struct name##_runner { \
        name##_runner() { \
            /* Tests will be run from main */ \
        } \
        void run() { \
            std::cout << "Running " #name "... "; \
            try { \
                name(); \
                std::cout << "PASSED\n"; \
            } catch (const std::exception& e) { \
                std::cout << "FAILED: " << e.what() << "\n"; \
                testsPassed = false; \
            } \
        } \
    } name##_instance; \
    void name()

#define ASSERT_TRUE(condition) \
    if (!(condition)) { \
        throw std::runtime_error("Assertion failed: " #condition); \
    }

#define ASSERT_FALSE(condition) \
    if (condition) { \
        throw std::runtime_error("Assertion failed: " #condition " should be false"); \
    }

#define ASSERT_EQ(expected, actual) \
    if ((expected) != (actual)) { \
        std::stringstream ss; \
        ss << "Assertion failed: expected " << (expected) << " but got " << (actual); \
        throw std::runtime_error(ss.str()); \
    }

// Initialize config for testing
void initTestConfig() {
    // Create a test config file with depth exactly at limit (10 levels)
    nlohmann::json testConfig = {
        {"level1", {
            {"level2", {
                {"level3", {
                    {"level4", {
                        {"level5", {
                            {"level6", {
                                {"level7", {
                                    {"level8", {
                                        {"level9", {
                                            {"value", "at_limit"}  // 10 levels total
                                        }}
                                    }}
                                }}
                            }}
                        }}
                    }}
                }}
            }}
        }},
        {"simple", "value"},
        {"number", 42},
        {"boolean", true}
    };
    
    // Write test config
    std::ofstream file("test_config_depth.json");
    file << testConfig.dump(2);
    file.close();
    
    // Load it
    Config::setSilentMode(false);
    Config::load("test_config_depth.json");
}

// Test 1: Normal depth within limits
TEST(NormalDepthAllowed) {
    Config::set("test.normal.depth.value", "test_value");
    auto result = Config::getString("test.normal.depth.value", "");
    ASSERT_EQ(std::string("test_value"), result);
    
    // Test existing deep value (9 levels + value = 10 levels total, at limit)
    auto deepValue = Config::getString("level1.level2.level3.level4.level5.level6.level7.level8.level9.value", "");
    ASSERT_EQ(std::string("at_limit"), deepValue);
}

// Test 2: Excessive depth rejected
TEST(ExcessiveDepthRejected) {
    // Try to set a value that exceeds the depth limit (11 levels)
    Config::set("l1.l2.l3.l4.l5.l6.l7.l8.l9.l10.l11.value", "should_fail");
    
    // The set should have failed, so getting the value should return default
    auto result = Config::getString("l1.l2.l3.l4.l5.l6.l7.l8.l9.l10.l11.value", "default");
    ASSERT_EQ(std::string("default"), result);
}

// Test 3: Key validation rejects invalid formats
TEST(KeyValidationWorks) {
    // Test various invalid key formats
    ASSERT_FALSE(Config::isValidConfigKey(".starts_with_dot"));
    ASSERT_FALSE(Config::isValidConfigKey("ends_with_dot."));
    ASSERT_FALSE(Config::isValidConfigKey("has..double..dots"));
    ASSERT_FALSE(Config::isValidConfigKey("has@invalid#chars!"));
    ASSERT_FALSE(Config::isValidConfigKey("")); // empty key
    
    // Test key that's too long
    std::string longKey(101, 'a');
    ASSERT_FALSE(Config::isValidConfigKey(longKey));
    
    // Test valid keys
    ASSERT_TRUE(Config::isValidConfigKey("valid.key.format"));
    ASSERT_TRUE(Config::isValidConfigKey("with_underscores"));
    ASSERT_TRUE(Config::isValidConfigKey("numbers123"));
    ASSERT_TRUE(Config::isValidConfigKey("single"));
}

// Test 4: Boundary conditions
TEST(BoundaryConditions) {
    // Test exactly at depth limit (10 levels)
    std::string keyAtLimit = "a.b.c.d.e.f.g.h.i.j";
    Config::set(keyAtLimit, "at_limit");
    auto result = Config::getString(keyAtLimit, "");
    ASSERT_EQ(std::string("at_limit"), result);
    
    // Test just over the limit (11 levels)
    std::string keyOverLimit = "a.b.c.d.e.f.g.h.i.j.k";
    Config::set(keyOverLimit, "over_limit");
    auto overResult = Config::getString(keyOverLimit, "default");
    ASSERT_EQ(std::string("default"), overResult);
    
    // Test empty key parts are ignored
    Config::set("has..empty...parts", "should_work");
    auto emptyPartsResult = Config::getString("has..empty...parts", "default");
    // This should fail validation due to double dots
    ASSERT_EQ(std::string("default"), emptyPartsResult);
    
    // Test single level key
    Config::set("single", "single_value");
    auto singleResult = Config::getString("single", "");
    ASSERT_EQ(std::string("single_value"), singleResult);
}

// Test 5: Different data types at depth
TEST(TypesAtDepth) {
    // Set different types at various depths
    Config::set("depth.test.string", "string_value");
    Config::set("depth.test.number", 12345);
    Config::set("depth.test.boolean", true);
    Config::set("depth.test.float", 3.14159);
    
    // Verify they can be retrieved correctly
    ASSERT_EQ(std::string("string_value"), Config::getString("depth.test.string", ""));
    ASSERT_EQ(12345, Config::getInt("depth.test.number", 0));
    ASSERT_TRUE(Config::getBool("depth.test.boolean", false));
    // Float comparison with tolerance
    float floatVal = Config::getFloat("depth.test.float", 0.0f);
    ASSERT_TRUE(floatVal > 3.14f && floatVal < 3.15f);
}

// Test 6: Circular reference protection
TEST(CircularReferenceProtection) {
    // Note: Creating actual circular references in JSON is not possible with nlohmann::json
    // as it prevents them at creation time. However, we can test that our traversal
    // doesn't get stuck in infinite loops with deep nesting
    
    // Create a deeply nested structure programmatically
    nlohmann::json circular;
    nlohmann::json* current = &circular;
    
    // Create a chain of objects
    for (int i = 0; i < 20; i++) {
        (*current)["next"] = nlohmann::json::object();
        current = &(*current)["next"];
    }
    
    // This tests that even with very deep nesting, we don't crash
    // The depth limit should prevent issues
    Config::set("circular.test", circular);
    
    // Try to access deep into the structure
    auto result = Config::get("circular.test.next.next.next.next.next.next.next.next.next.next.next", nullptr);
    // Should fail due to depth limit
    ASSERT_TRUE(result.is_null());
}

// Test 7: Path creation respects depth limits
TEST(PathCreationRespectsDepth) {
    // Try to create a path that exceeds depth limit
    Config::set("create.path.that.is.way.too.deep.to.be.allowed.beyond.limit", "should_fail");
    
    // Verify the path wasn't created
    auto result = Config::getString("create.path.that.is.way.too.deep.to.be.allowed.beyond.limit", "not_found");
    ASSERT_EQ(std::string("not_found"), result);
    
    // Verify partial path wasn't created either
    auto partial = Config::get("create.path.that", nullptr);
    ASSERT_TRUE(partial.is_null());
}

// Test 8: Custom depth limits
TEST(CustomDepthLimits) {
    // Test that navigateToKey respects custom depth limits
    // This requires accessing the private method, so we test indirectly
    // by observing the behavior of get/set operations
    
    // First check that the config is properly loaded
    auto simpleValue = Config::getString("simple", "");
    if (simpleValue.empty()) {
        std::cerr << "\nERROR: Config not properly loaded!" << std::endl;
    }
    
    // The default limit is 10, so this should work (9 levels) - use unique path
    std::string key9Levels = "x1.x2.x3.x4.x5.x6.x7.x8.x9";
    Config::set(key9Levels, "nine_levels");
    ASSERT_EQ(std::string("nine_levels"), Config::getString(key9Levels, ""));
    
    // This is at the limit (10 levels = 9 dots) - use unique path
    std::string key10Levels = "y1.y2.y3.y4.y5.y6.y7.y8.y9.y10";
    Config::set(key10Levels, "ten_levels");
    auto result10 = Config::getString(key10Levels, "default");
    // Debug output
    if (result10 != "ten_levels") {
        std::cerr << "\nDEBUG: 10-level key failed. Key: " << key10Levels 
                  << ", dots: " << std::count(key10Levels.begin(), key10Levels.end(), '.') 
                  << ", result: " << result10 << std::endl;
        std::cerr << "isValidConfigKey: " << Config::isValidConfigKey(key10Levels) << std::endl;
        
        // Check config state
        auto configJson = Config::getConfig();
        std::cerr << "Config is object: " << configJson.is_object() << std::endl;
        std::cerr << "Config size: " << configJson.size() << std::endl;
        
        // Check if t1 already exists
        if (configJson.contains("t1")) {
            std::cerr << "t1 already exists in config!" << std::endl;
        }
        
        // Show what keys exist at root level
        std::cerr << "Root keys in config: ";
        for (auto& [key, value] : configJson.items()) {
            std::cerr << key << " ";
        }
        std::cerr << std::endl;
    }
    ASSERT_EQ(std::string("ten_levels"), result10);
    
    // This exceeds the limit (11 levels = 10 dots)
    std::string key11Levels = "z1.z2.z3.z4.z5.z6.z7.z8.z9.z10.z11";
    Config::set(key11Levels, "eleven_levels");
    ASSERT_EQ(std::string("default"), Config::getString(key11Levels, "default"));
}

// Cleanup function
void cleanup() {
    std::remove("test_config_depth.json");
}

int main() {
    std::cout << "=== Config Depth Validation Tests ===" << std::endl;
    
    try {
        initTestConfig();
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize test config: " << e.what() << std::endl;
        return 1;
    }
    
    // Run tests manually after initialization
    NormalDepthAllowed_instance.run();
    ExcessiveDepthRejected_instance.run();
    KeyValidationWorks_instance.run();
    BoundaryConditions_instance.run();
    TypesAtDepth_instance.run();
    CircularReferenceProtection_instance.run();
    PathCreationRespectsDepth_instance.run();
    CustomDepthLimits_instance.run();
    
    cleanup();
    
    if (testsPassed) {
        std::cout << "\nAll tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "\nSome tests failed!" << std::endl;
        return 1;
    }
}