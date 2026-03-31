#include "allocator.hpp"
#include <iostream>
#include <string>
#include <map>

int main() {
    std::size_t poolSize;
    std::cin >> poolSize;

    TLSFAllocator allocator(poolSize);

    int numOperations;
    std::cin >> numOperations;

    std::map<int, void*> allocations;

    for (int i = 0; i < numOperations; ++i) {
        std::string operation;
        std::cin >> operation;

        if (operation == "allocate") {
            int id;
            std::size_t size;
            std::cin >> id >> size;

            void* ptr = allocator.allocate(size);
            if (ptr) {
                allocations[id] = ptr;
                std::cout << "allocate " << id << " success" << std::endl;
            } else {
                std::cout << "allocate " << id << " fail" << std::endl;
            }
        } else if (operation == "deallocate") {
            int id;
            std::cin >> id;

            auto it = allocations.find(id);
            if (it != allocations.end()) {
                allocator.deallocate(it->second);
                allocations.erase(it);
                std::cout << "deallocate " << id << " success" << std::endl;
            } else {
                std::cout << "deallocate " << id << " fail" << std::endl;
            }
        } else if (operation == "max_available") {
            std::size_t maxSize = allocator.getMaxAvailableBlockSize();
            std::cout << "max_available " << maxSize << std::endl;
        }
    }

    return 0;
}
