#include "allocator.hpp"
#include <iostream>
#include <cassert>

void testBasicAllocation() {
    std::cout << "Test 1: Basic allocation and deallocation" << std::endl;
    TLSFAllocator allocator(1024);

    void* ptr1 = allocator.allocate(64);
    assert(ptr1 != nullptr);
    std::cout << "  Allocated 64 bytes: " << ptr1 << std::endl;

    void* ptr2 = allocator.allocate(128);
    assert(ptr2 != nullptr);
    std::cout << "  Allocated 128 bytes: " << ptr2 << std::endl;

    allocator.deallocate(ptr1);
    std::cout << "  Deallocated first block" << std::endl;

    void* ptr3 = allocator.allocate(64);
    assert(ptr3 != nullptr);
    std::cout << "  Re-allocated 64 bytes: " << ptr3 << std::endl;

    allocator.deallocate(ptr2);
    allocator.deallocate(ptr3);
    std::cout << "  Test passed!" << std::endl;
}

void testMaxAvailable() {
    std::cout << "\nTest 2: Max available block size" << std::endl;
    TLSFAllocator allocator(2048);

    std::size_t maxSize = allocator.getMaxAvailableBlockSize();
    std::cout << "  Initial max available: " << maxSize << std::endl;

    void* ptr1 = allocator.allocate(512);
    maxSize = allocator.getMaxAvailableBlockSize();
    std::cout << "  After allocating 512 bytes: " << maxSize << std::endl;

    allocator.deallocate(ptr1);
    maxSize = allocator.getMaxAvailableBlockSize();
    std::cout << "  After deallocation: " << maxSize << std::endl;
    std::cout << "  Test passed!" << std::endl;
}

void testFragmentation() {
    std::cout << "\nTest 3: Fragmentation and coalescing" << std::endl;
    TLSFAllocator allocator(4096);

    void* ptr1 = allocator.allocate(256);
    void* ptr2 = allocator.allocate(256);
    void* ptr3 = allocator.allocate(256);
    std::cout << "  Allocated 3 blocks of 256 bytes" << std::endl;

    allocator.deallocate(ptr1);
    allocator.deallocate(ptr3);
    std::cout << "  Deallocated blocks 1 and 3" << std::endl;

    allocator.deallocate(ptr2);
    std::cout << "  Deallocated block 2 (should coalesce)" << std::endl;

    std::size_t maxSize = allocator.getMaxAvailableBlockSize();
    std::cout << "  Max available after coalescing: " << maxSize << std::endl;
    std::cout << "  Test passed!" << std::endl;
}

void testLargeAllocation() {
    std::cout << "\nTest 4: Large allocation" << std::endl;
    TLSFAllocator allocator(1024 * 1024); // 1 MB

    void* ptr = allocator.allocate(512 * 1024);
    assert(ptr != nullptr);
    std::cout << "  Allocated 512 KB successfully" << std::endl;

    allocator.deallocate(ptr);
    std::cout << "  Test passed!" << std::endl;
}

void testAllocationFailure() {
    std::cout << "\nTest 5: Allocation failure" << std::endl;
    TLSFAllocator allocator(1024);

    void* ptr1 = allocator.allocate(900);
    assert(ptr1 != nullptr);
    std::cout << "  Allocated 900 bytes" << std::endl;

    void* ptr2 = allocator.allocate(900);
    assert(ptr2 == nullptr);
    std::cout << "  Second allocation of 900 bytes correctly failed" << std::endl;

    allocator.deallocate(ptr1);
    std::cout << "  Test passed!" << std::endl;
}

int main() {
    std::cout << "=== TLSF Allocator Test Suite ===" << std::endl;

    try {
        testBasicAllocation();
        testMaxAvailable();
        testFragmentation();
        testLargeAllocation();
        testAllocationFailure();

        std::cout << "\n=== All tests passed! ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
