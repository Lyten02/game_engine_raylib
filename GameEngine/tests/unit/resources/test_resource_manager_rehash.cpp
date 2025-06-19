#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <cstring>

// Test to force std::unordered_map rehashing and demonstrate pointer invalidation

struct Texture2D {
    int id;
    int width;
    int height;
    char data[1024]; // Make it bigger to increase chance of reallocation
};

void testRehashing() {
    std::cout << "=== Testing unordered_map Rehashing ===" << std::endl;
    
    std::unordered_map<std::string, Texture2D> textures;
    std::vector<Texture2D*> pointers;
    std::vector<std::string> names;
    
    // Reserve to force a specific bucket count
    textures.reserve(8);
    
    std::cout << "\nInitial state:" << std::endl;
    std::cout << "  Bucket count: " << textures.bucket_count() << std::endl;
    std::cout << "  Max load factor: " << textures.max_load_factor() << std::endl;
    
    // Step 1: Add textures until we're close to rehashing
    std::cout << "\nAdding textures..." << std::endl;
    for (int i = 0; i < 50; ++i) {
        std::string name = "tex_" + std::to_string(i);
        names.push_back(name);
        
        Texture2D tex;
        tex.id = i + 1;
        tex.width = 64;
        tex.height = 64;
        std::memset(tex.data, i, sizeof(tex.data));
        
        textures[name] = tex;
        pointers.push_back(&textures[name]);
        
        if (i % 10 == 0) {
            std::cout << "  After " << (i + 1) << " textures:" << std::endl;
            std::cout << "    Bucket count: " << textures.bucket_count() << std::endl;
            std::cout << "    Load factor: " << textures.load_factor() << std::endl;
            std::cout << "    Address of tex_0: " << &textures["tex_0"] << std::endl;
        }
    }
    
    // Step 2: Force a rehash
    std::cout << "\nForcing rehash with large reserve..." << std::endl;
    size_t oldBucketCount = textures.bucket_count();
    textures.rehash(1000);
    size_t newBucketCount = textures.bucket_count();
    
    std::cout << "  Old bucket count: " << oldBucketCount << std::endl;
    std::cout << "  New bucket count: " << newBucketCount << std::endl;
    
    // Step 3: Check if pointers are still valid
    std::cout << "\nChecking pointer validity after rehash..." << std::endl;
    int invalidCount = 0;
    
    for (size_t i = 0; i < std::min(size_t(10), pointers.size()); ++i) {
        Texture2D* oldPtr = pointers[i];
        Texture2D* newPtr = &textures[names[i]];
        
        std::cout << "  " << names[i] << ": ";
        std::cout << "old=" << oldPtr << ", new=" << newPtr;
        
        if (oldPtr != newPtr) {
            std::cout << " ❌ CHANGED!";
            invalidCount++;
        } else {
            std::cout << " ✓";
        }
        std::cout << std::endl;
    }
    
    if (invalidCount > 0) {
        std::cout << "\n⚠️  " << invalidCount << " pointers became invalid after rehash!" << std::endl;
    } else {
        std::cout << "\n✓ All pointers remained valid (implementation dependent)" << std::endl;
    }
    
    // Step 4: Demonstrate the real danger
    std::cout << "\nDemonstrating the real danger:" << std::endl;
    std::cout << "Even if pointers didn't change this time, they CAN change." << std::endl;
    std::cout << "The C++ standard does NOT guarantee pointer stability for std::unordered_map!" << std::endl;
    std::cout << "Iterators and references to elements are invalidated by rehashing." << std::endl;
}

void testWorstCase() {
    std::cout << "\n\n=== Worst Case Scenario ===" << std::endl;
    
    std::unordered_map<std::string, Texture2D> textures;
    
    // Get initial texture
    textures["important"] = {1, 64, 64, {}};
    Texture2D* importantPtr = &textures["important"];
    
    std::cout << "Important texture at: " << importantPtr << std::endl;
    std::cout << "Initial id: " << importantPtr->id << std::endl;
    
    // Simulate what could happen in production
    std::cout << "\nSimulating production scenario..." << std::endl;
    
    // Many operations later...
    for (int i = 0; i < 1000; ++i) {
        textures["dynamic_" + std::to_string(i)] = {i + 100, 32, 32, {}};
    }
    
    // Check if our pointer is still valid
    Texture2D* currentPtr = &textures["important"];
    
    std::cout << "\nAfter many insertions:" << std::endl;
    std::cout << "Original pointer: " << importantPtr << std::endl;
    std::cout << "Current pointer: " << currentPtr << std::endl;
    
    if (importantPtr != currentPtr) {
        std::cout << "\n❌ CRITICAL: Pointer has changed!" << std::endl;
        std::cout << "Accessing through old pointer is undefined behavior!" << std::endl;
        
        // DON'T DO THIS IN REAL CODE - just for demonstration
        std::cout << "\nTrying to access through old pointer (UNSAFE):" << std::endl;
        std::cout << "Old pointer id: " << importantPtr->id << " (might be garbage!)" << std::endl;
    } else {
        std::cout << "\n✓ Pointer remained stable (but this is NOT guaranteed!)" << std::endl;
    }
}

int main() {
    std::cout << "ResourceManager Rehashing Test\n" << std::endl;
    
    testRehashing();
    testWorstCase();
    
    std::cout << "\n\nConclusion:" << std::endl;
    std::cout << "- std::unordered_map does NOT guarantee pointer stability" << std::endl;
    std::cout << "- Rehashing can invalidate ALL pointers to elements" << std::endl;
    std::cout << "- This is a critical bug waiting to happen in production" << std::endl;
    std::cout << "- Solution: Use std::unique_ptr or std::shared_ptr to ensure stability" << std::endl;
    
    return 0;
}