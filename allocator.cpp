#include "allocator.hpp"
#include <cstring>
#include <algorithm>
#include <bit>

// 最小块大小：必须至少能容纳 FreeBlock 结构
// 使用 BlockHeader 加上两个指针的大小
static constexpr std::size_t MIN_BLOCK_SIZE = 64; // FreeBlock 的大致大小
static constexpr std::size_t ALIGNMENT = 16; // 16 字节对齐

// 辅助函数：向上对齐
static inline std::size_t alignUp(std::size_t size, std::size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

// 辅助函数：找到最高位的位置 (0-indexed)
static inline int my_fls(std::uint32_t x) {
    if (x == 0) return -1;
    return 31 - __builtin_clz(x);
}

// 辅助函数：找到最低位的位置 (0-indexed)
static inline int my_ffs(std::uint32_t x) {
    if (x == 0) return -1;
    return __builtin_ctz(x);
}

TLSFAllocator::TLSFAllocator(std::size_t memoryPoolSize)
    : poolSize(memoryPoolSize) {
    initializeMemoryPool(memoryPoolSize);
}

TLSFAllocator::~TLSFAllocator() {
    if (memoryPool) {
        delete[] static_cast<char*>(memoryPool);
    }
}

void TLSFAllocator::initializeMemoryPool(std::size_t size) {
    // 分配内存池
    memoryPool = new char[size];
    poolSize = size;

    // 初始化位图
    index.fliBitmap = 0;
    for (int i = 0; i < FLI_SIZE; ++i) {
        index.sliBitmaps[i] = 0;
        for (int j = 0; j < SLI_SIZE; ++j) {
            index.freeLists[i][j] = nullptr;
        }
    }

    // 创建初始的大块
    // 块头存储在内存池的开始处
    FreeBlock* initialBlock = reinterpret_cast<FreeBlock*>(memoryPool);
    // data 指向块本身的起始位置（包括头部）
    initialBlock->data = memoryPool;
    initialBlock->size = size; // 总大小包括头部
    initialBlock->isFree = true;
    initialBlock->prevPhysBlock = nullptr;
    initialBlock->nextPhysBlock = nullptr;
    initialBlock->prevFree = nullptr;
    initialBlock->nextFree = nullptr;

    insertFreeBlock(initialBlock);
}

void TLSFAllocator::mappingFunction(std::size_t size, int& fli, int& sli) {
    if (size == 0) {
        fli = 0;
        sli = 0;
        return;
    }

    // 计算 FLI: floor(log2(size))
    fli = my_fls(static_cast<std::uint32_t>(size));

    // 确保 fli 在有效范围内
    if (fli >= FLI_SIZE) {
        fli = FLI_SIZE - 1;
    }

    // 计算 SLI
    // divisions = min(2^FLI, 16) 根据 hint
    std::size_t range_start = 1ULL << fli;
    std::size_t divisions = std::min(range_start, static_cast<std::size_t>(SLI_SIZE));

    if (divisions == 0) {
        divisions = 1;
    }

    std::size_t step = range_start / divisions;
    if (step == 0) {
        step = 1;
    }

    sli = static_cast<int>((size - range_start) / step);

    // 确保 sli 在有效范围内
    if (sli >= SLI_SIZE) {
        sli = SLI_SIZE - 1;
    }
    if (sli < 0) {
        sli = 0;
    }
}

void TLSFAllocator::insertFreeBlock(FreeBlock* block) {
    int fli, sli;
    mappingFunction(block->size, fli, sli);

    // 插入到链表头部
    block->nextFree = index.freeLists[fli][sli];
    block->prevFree = nullptr;

    if (index.freeLists[fli][sli]) {
        index.freeLists[fli][sli]->prevFree = block;
    }

    index.freeLists[fli][sli] = block;

    // 更新位图
    index.fliBitmap |= (1U << fli);
    index.sliBitmaps[fli] |= (1U << sli);
}

void TLSFAllocator::removeFreeBlock(FreeBlock* block) {
    int fli, sli;
    mappingFunction(block->size, fli, sli);

    // 从链表中移除
    if (block->prevFree) {
        block->prevFree->nextFree = block->nextFree;
    } else {
        index.freeLists[fli][sli] = block->nextFree;
    }

    if (block->nextFree) {
        block->nextFree->prevFree = block->prevFree;
    }

    // 如果链表为空，清除位图
    if (index.freeLists[fli][sli] == nullptr) {
        index.sliBitmaps[fli] &= ~(1U << sli);
        if (index.sliBitmaps[fli] == 0) {
            index.fliBitmap &= ~(1U << fli);
        }
    }
}

TLSFAllocator::FreeBlock* TLSFAllocator::findSuitableBlock(std::size_t size) {
    int fli, sli;
    mappingFunction(size, fli, sli);

    // 查找当前 sli 或更大的槽
    std::uint16_t slBitmap = index.sliBitmaps[fli] & (~0U << sli);

    if (slBitmap == 0) {
        // 当前 fli 没有合适的块，查找更大的 fli
        std::uint32_t flBitmap = index.fliBitmap & (~0U << (fli + 1));
        if (flBitmap == 0) {
            return nullptr; // 没有合适的块
        }

        fli = my_ffs(flBitmap);
        slBitmap = index.sliBitmaps[fli];
    }

    sli = my_ffs(slBitmap);
    return index.freeLists[fli][sli];
}

void TLSFAllocator::splitBlock(FreeBlock* block, std::size_t size) {
    std::size_t remainingSize = block->size - size;

    // 只有剩余块足够大时才分割
    if (remainingSize >= MIN_BLOCK_SIZE) {
        // 创建新的空闲块
        FreeBlock* newBlock = reinterpret_cast<FreeBlock*>(
            static_cast<char*>(block->data) + size
        );

        newBlock->data = reinterpret_cast<void*>(newBlock);
        newBlock->size = remainingSize;
        newBlock->isFree = true;
        newBlock->prevPhysBlock = reinterpret_cast<BlockHeader*>(block);
        newBlock->nextPhysBlock = block->nextPhysBlock;
        newBlock->prevFree = nullptr;
        newBlock->nextFree = nullptr;

        if (block->nextPhysBlock) {
            block->nextPhysBlock->prevPhysBlock = reinterpret_cast<BlockHeader*>(newBlock);
        }

        block->nextPhysBlock = reinterpret_cast<BlockHeader*>(newBlock);
        block->size = size;

        insertFreeBlock(newBlock);
    }
}

void TLSFAllocator::mergeAdjacentFreeBlocks(FreeBlock* block) {
    // 尝试与后一个块合并
    if (block->nextPhysBlock && block->nextPhysBlock->isFree) {
        FreeBlock* nextBlock = reinterpret_cast<FreeBlock*>(block->nextPhysBlock);
        removeFreeBlock(nextBlock);

        block->size += nextBlock->size;
        block->nextPhysBlock = nextBlock->nextPhysBlock;

        if (block->nextPhysBlock) {
            block->nextPhysBlock->prevPhysBlock = reinterpret_cast<BlockHeader*>(block);
        }
    }

    // 尝试与前一个块合并
    if (block->prevPhysBlock && block->prevPhysBlock->isFree) {
        FreeBlock* prevBlock = reinterpret_cast<FreeBlock*>(block->prevPhysBlock);
        removeFreeBlock(prevBlock);

        prevBlock->size += block->size;
        prevBlock->nextPhysBlock = block->nextPhysBlock;

        if (block->nextPhysBlock) {
            block->nextPhysBlock->prevPhysBlock = reinterpret_cast<BlockHeader*>(prevBlock);
        }

        block = prevBlock;
    }

    insertFreeBlock(block);
}

void* TLSFAllocator::allocate(std::size_t size) {
    if (size == 0) return nullptr;

    // 对齐大小并确保至少为最小块大小
    size = alignUp(size, ALIGNMENT);
    size = std::max(size, MIN_BLOCK_SIZE);

    // 查找合适的块
    FreeBlock* block = findSuitableBlock(size);
    if (!block) {
        return nullptr; // 内存不足
    }

    // 从空闲列表中移除
    removeFreeBlock(block);

    // 分割块（如果需要）
    splitBlock(block, size);

    // 标记为已分配
    block->isFree = false;

    // 返回数据指针（跳过头部）
    return static_cast<char*>(block->data) + sizeof(BlockHeader);
}

void TLSFAllocator::deallocate(void* ptr) {
    if (!ptr) return;

    // 获取块头
    BlockHeader* header = reinterpret_cast<BlockHeader*>(
        static_cast<char*>(ptr) - sizeof(BlockHeader)
    );

    // 将块转换为空闲块
    FreeBlock* block = reinterpret_cast<FreeBlock*>(header);
    block->isFree = true;
    block->prevFree = nullptr;
    block->nextFree = nullptr;

    // 合并相邻的空闲块
    mergeAdjacentFreeBlocks(block);
}

void* TLSFAllocator::getMemoryPoolStart() const {
    return memoryPool;
}

std::size_t TLSFAllocator::getMemoryPoolSize() const {
    return poolSize;
}

std::size_t TLSFAllocator::getMaxAvailableBlockSize() const {
    // 从最高的 FLI 开始查找
    if (index.fliBitmap == 0) {
        return 0;
    }

    int fli = my_fls(index.fliBitmap);
    int sli = my_fls(index.sliBitmaps[fli]);

    FreeBlock* block = index.freeLists[fli][sli];
    std::size_t maxSize = 0;

    // 遍历最大的链表找到最大块
    while (block) {
        if (block->size > maxSize) {
            maxSize = block->size;
        }
        block = block->nextFree;
    }

    // 减去头部大小，返回可用大小
    return maxSize > sizeof(BlockHeader) ? maxSize - sizeof(BlockHeader) : 0;
}
