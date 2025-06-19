#include <iostream>
#include <vector>
#include <thread>
#include <unordered_map>
#include <string>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <atomic>

// Simple demonstration of the dangling pointer problem with std::unordered_map

struct Texture2D {
    int id;
    int width;
    int height;
    int mipmaps;
    int format;
};

class ResourceManagerSimplified {
private:
    std::unordered_map<std::string, Texture2D> textures;
    
public:
    Texture2D* loadTexture(const std::string& name) {
        Texture2D texture;
        texture.id = textures.size() + 1;
        texture.width = 64;
        texture.height = 64;
        texture.mipmaps = 1;
        texture.format = 7; // PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
        
        textures[name] = texture;
        return &textures[name];
    }
    
    Texture2D* getTexture(const std::string& name) {
        auto it = textures.find(name);
        if (it != textures.end()) {
            return &it->second;
        }
        return nullptr;
    }
    
    size_t getTextureCount() const {
        return textures.size();
    }
    
    size_t getBucketCount() const {
        return textures.bucket_count();
    }
    
    void clear() {
        textures.clear();
    }
};

void demonstrateDanglingPointer() {
    std::cout << "=== Demonstrating Dangling Pointer Problem ===" << std::endl;
    
    ResourceManagerSimplified manager;
    
    // Step 1: Load some textures and store their pointers
    std::vector<Texture2D*> texturePointers;
    
    std::cout << "\nStep 1: Loading initial textures..." << std::endl;
    for (int i = 0; i < 5; ++i) {
        std::string name = "texture_" + std::to_string(i);
        Texture2D* ptr = manager.loadTexture(name);
        texturePointers.push_back(ptr);
        std::cout << "  Loaded " << name << " at address: " << ptr 
                  << " (id=" << ptr->id << ")" << std::endl;
    }
    
    // Step 2: Verify all pointers are valid
    std::cout << "\nStep 2: Verifying pointers are valid..." << std::endl;
    bool allValid = true;
    for (size_t i = 0; i < texturePointers.size(); ++i) {
        Texture2D* ptr = texturePointers[i];
        if (ptr->id != static_cast<int>(i + 1)) {
            allValid = false;
            std::cout << "  ERROR: texture_" << i << " has incorrect id!" << std::endl;
        }
    }
    if (allValid) {
        std::cout << "  ✓ All pointers are valid" << std::endl;
    }
    
    // Step 3: Cause map reallocation by adding many more textures
    std::cout << "\nStep 3: Adding many textures to trigger reallocation..." << std::endl;
    std::cout << "  Initial bucket count: " << manager.getBucketCount() << std::endl;
    
    // Force reallocation by adding many textures
    for (int i = 5; i < 100; ++i) {
        std::string name = "texture_" + std::to_string(i);
        manager.loadTexture(name);
    }
    
    std::cout << "  Final bucket count: " << manager.getBucketCount() << std::endl;
    std::cout << "  Total textures loaded: " << manager.getTextureCount() << std::endl;
    
    // Step 4: Check if original pointers are still valid
    std::cout << "\nStep 4: Checking if original pointers are still valid..." << std::endl;
    
    // First, let's see if the addresses changed
    std::cout << "  Comparing addresses:" << std::endl;
    for (int i = 0; i < 5; ++i) {
        std::string name = "texture_" + std::to_string(i);
        Texture2D* currentPtr = manager.getTexture(name);
        Texture2D* originalPtr = texturePointers[i];
        
        std::cout << "    " << name << ": original=" << originalPtr 
                  << ", current=" << currentPtr;
        
        if (originalPtr != currentPtr) {
            std::cout << " ⚠️  ADDRESSES CHANGED!" << std::endl;
        } else {
            std::cout << " ✓" << std::endl;
        }
    }
    
    // Now let's try to access through the original pointers (DANGEROUS!)
    std::cout << "\n  Attempting to access through original pointers:" << std::endl;
    std::cout << "  WARNING: This may crash or show garbage data!" << std::endl;
    
    for (size_t i = 0; i < texturePointers.size() && i < 3; ++i) {
        Texture2D* ptr = texturePointers[i];
        std::cout << "    texture_" << i << " via old pointer: ";
        
        // This is UNSAFE and demonstrates the problem
        // In a real scenario, this could crash or return garbage
        std::cout << "id=" << ptr->id 
                  << ", width=" << ptr->width 
                  << ", height=" << ptr->height << std::endl;
    }
    
    std::cout << "\n=== Demonstration Complete ===" << std::endl;
    std::cout << "\nThe dangling pointer problem occurs because:" << std::endl;
    std::cout << "1. std::unordered_map may reallocate its internal storage" << std::endl;
    std::cout << "2. When this happens, all elements move to new memory locations" << std::endl;
    std::cout << "3. Previously returned pointers become invalid (dangling)" << std::endl;
    std::cout << "4. Accessing these pointers leads to undefined behavior" << std::endl;
}

void demonstrateConcurrentAccessProblem() {
    std::cout << "\n\n=== Demonstrating Concurrent Access Problem ===" << std::endl;
    
    ResourceManagerSimplified manager;
    
    // Load initial texture
    Texture2D* texture1 = manager.loadTexture("player");
    std::cout << "Initial texture 'player' at: " << texture1 << std::endl;
    
    // Simulate concurrent access
    std::atomic<bool> stop{false};
    std::vector<std::thread> threads;
    
    // Reader thread - continuously accesses the texture
    threads.emplace_back([&]() {
        int accessCount = 0;
        while (!stop.load()) {
            // This pointer might become invalid!
            if (texture1->id == 1 && texture1->width == 64) {
                accessCount++;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
        std::cout << "Reader thread made " << accessCount << " successful accesses" << std::endl;
    });
    
    // Writer thread - adds more textures
    threads.emplace_back([&]() {
        for (int i = 0; i < 50; ++i) {
            manager.loadTexture("dynamic_" + std::to_string(i));
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        std::cout << "Writer thread added 50 textures" << std::endl;
    });
    
    // Wait for writer to finish
    threads[1].join();
    
    // Stop reader
    stop.store(true);
    threads[0].join();
    
    // Check if pointer is still valid
    Texture2D* currentPlayer = manager.getTexture("player");
    std::cout << "\nOriginal 'player' pointer: " << texture1 << std::endl;
    std::cout << "Current 'player' pointer: " << currentPlayer << std::endl;
    
    if (texture1 != currentPlayer) {
        std::cout << "⚠️  DANGER: The pointer has changed! Original pointer is now dangling!" << std::endl;
    }
}

int main() {
    std::cout << "ResourceManager Dangling Pointer Test\n" << std::endl;
    
    // Run demonstrations
    demonstrateDanglingPointer();
    demonstrateConcurrentAccessProblem();
    
    std::cout << "\n✅ Test completed (if you see this, we didn't crash!)" << std::endl;
    
    return 0;
}