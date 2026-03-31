#include "allocator.hpp"
#include <iostream>
#include <iomanip>

int main() {
    std::cout << "Creating allocator with 1024 bytes" << std::endl;
    TLSFAllocator allocator(1024);

    std::cout << "Calling allocate(64)..." << std::endl;

    // Manually check what mapping we'd get
    int fli = 0, sli = 0;
    std::size_t size = 64;

    // Simulate mappingFunction for size=64
    // fls(64) = 6
    fli = 6;
    std::size_t range_start = 1ULL << fli; // 64
    std::size_t divisions = std::min(range_start, static_cast<std::size_t>(16)); // min(64, 16) = 16
    std::size_t step = range_start / divisions; // 64 / 16 = 4
    sli = (size - range_start) / step; // (64 - 64) / 4 = 0

    std::cout << "For size 64: FLI=" << fli << ", SLI=" << sli << std::endl;

    // Simulate mappingFunction for size=1024
    fli = 10;
    range_start = 1ULL << fli; // 1024
    divisions = std::min(range_start, static_cast<std::size_t>(16)); // min(1024, 16) = 16
    step = range_start / divisions; // 1024 / 16 = 64
    sli = (1024 - range_start) / step; // (1024 - 1024) / 64 = 0

    std::cout << "For size 1024: FLI=" << fli << ", SLI=" << sli << std::endl;

    void* ptr = allocator.allocate(64);
    std::cout << "Result: " << ptr << std::endl;

    return 0;
}
