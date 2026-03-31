#include "allocator.hpp"
#include <iostream>

// Test the mapping function
void testMapping() {
    TLSFAllocator allocator(1024);

    // Test finding suitable block for size 64
    std::cout << "Testing findSuitableBlock for size 64..." << std::endl;

    // The initial block should be 1024 bytes
    // After header overhead (assuming about 40 bytes), we have ~984 bytes available
    // For size 64:
    // FLI = floor(log2(64)) = 6
    // Initial block size 1024:
    // FLI = floor(log2(1024)) = 10

    std::cout << "Size 1: FLI=" << __builtin_clz(1) - __builtin_clz(1) << std::endl;
    std::cout << "Size 64: FLI=" << (31 - __builtin_clz(64)) << std::endl;
    std::cout << "Size 1024: FLI=" << (31 - __builtin_clz(1024)) << std::endl;

    std::cout << "\nAllocating 64 bytes..." << std::endl;
    void* ptr = allocator.allocate(64);
    std::cout << "Result: " << ptr << std::endl;
}

int main() {
    testMapping();
    return 0;
}
