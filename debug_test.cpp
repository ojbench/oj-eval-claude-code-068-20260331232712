#include "allocator.hpp"
#include <iostream>

int main() {
    std::cout << "Creating allocator with 1024 bytes" << std::endl;
    TLSFAllocator allocator(1024);

    std::cout << "Pool start: " << allocator.getMemoryPoolStart() << std::endl;
    std::cout << "Pool size: " << allocator.getMemoryPoolSize() << std::endl;
    std::cout << "Max available: " << allocator.getMaxAvailableBlockSize() << std::endl;

    std::cout << "\nAttempting to allocate 64 bytes..." << std::endl;
    void* ptr1 = allocator.allocate(64);
    std::cout << "Result: " << ptr1 << std::endl;

    if (ptr1) {
        std::cout << "Max available after allocation: " << allocator.getMaxAvailableBlockSize() << std::endl;
    }

    return 0;
}
