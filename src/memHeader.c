/**
 * @file memHeader.c
 * @brief Core memory allocator implementation
 * @author Robkoo
 * @date 19.06.2025
 * @version 0.0.1
 * @since 0.0.1
 * 
 * Contains the main memory pool initialization and management functions
 * for the RMA (Robkoo's Memory Allocator) handle-based memory allocator.
 * Handles bitmap tracking, offset calculations, and memory pool setup.
 */

#include "memHeader.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void* rma_memHeaderInit(size_t totalSize, size_t blockSize){
    // Allocate the desired memory pool
    void *memPool = malloc(totalSize);
    if (memPool == NULL) return NULL;

    // initialize the header at the start of the pool
    struct rma_mem_header_t *header = (struct rma_mem_header_t*)memPool;

    // Aproximate block sizing
    size_t const headerSize = sizeof(struct rma_mem_header_t);
    size_t const maxPossibleBlocks = (totalSize - headerSize) / blockSize;

    // Calculate layout offsets
    size_t const bitmapSize = (maxPossibleBlocks + 31) / 32 * sizeof(uint32_t);
    size_t const handleTableSize = maxPossibleBlocks * sizeof(uint32_t);

    // initialize info of the struct
    header->totalSize = totalSize;
    header->usedSize = headerSize;
    header->numAllocated = 0;
    header->nextHandle = 1; // Handles start at 1 (0 = invalid)

    // Initialize offsets
    header->bitmapOffset = headerSize;
    header->handleTableOffset = headerSize + bitmapSize;
    header->dataOffset = headerSize + bitmapSize + handleTableSize;

    // Fix the numBlocks calculation
    size_t const remainingSpace = totalSize - header->dataOffset;
    header->numBlocks = remainingSpace / blockSize;

    // Clear the bitmap
    memset((char*)memPool + header->bitmapOffset, 0, (header->numBlocks + 7) / 8);

    return header;
}

void rma_displayMemInfo(struct rma_mem_header_t *header){
    if (!header){
        printf("RMA: Header is NULL\n");
        return;
    }

    printf("=== RMA Info === \n");
    printf("Total size: %zu bytes (%.4f MiB)\n",
        header->totalSize, (double)header->totalSize / (1024.0 * 1024.0));
        printf("Used Size: %zu bytes (%.4f MiB)\n", 
            header->usedSize, (double)header->usedSize / (1024.0 * 1024.0));
     printf("Free Size: %zu bytes (%.4f MiB)\n", 
            header->totalSize - header->usedSize, 
            (double)(header->totalSize - header->usedSize) / (1024.0 * 1024.0));
     printf("Total Blocks: %zu\n", header->numBlocks);
     printf("Allocated Blocks: %zu\n", header->numAllocated);
     printf("Free Blocks: %zu\n", header->numBlocks - header->numAllocated);
     printf("Next Handle: %u\n", header->nextHandle);
     printf("Usage: %.4f%%\n", 
            ((double)header->usedSize / header->totalSize) * 100.0);
     printf("================================\n");
}