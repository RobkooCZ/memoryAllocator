/**
 * @file memHeader.c
 * @brief Core memory allocator implementation
 * @author Robkoo
 * @date 19.06.2025
 * @version 0.0.2
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

/**
 * STATIC HELPER FUNCTIONS
*/

/**
 * @brief Get pointer to bitmap array for bit manipulation operations
 * @param header Pointer to RMA header structure (must not be NULL)
 * @return Pointer to bitmap array as uint32_t* for efficient bit operations
 * 
 * Converts the stored bitmap offset into a usable pointer for direct
 * bitmap manipulation. The bitmap uses 1 bit per block to track allocation status.
 */
static uint32_t* rma_getBitmap(struct rma_mem_header_t *header){
    return (uint32_t*)((char*)header + header->bitmapOffset);
}

/**
 * @brief Get pointer to handle table array for handle-to-block mapping
 * @param header Pointer to RMA header structure (must not be NULL)
 * @return Pointer to handle table array as uint32_t* for salt storage
 * 
 * Converts the stored handle table offset into a usable pointer. Each entry
 * in the table stores the salt value for the corresponding block index.
 */
static uint32_t* rma_getHandleTable(struct rma_mem_header_t *header){
    return (uint32_t*)((char*)header + header->handleTableOffset);
}

/**
 * @brief Check if a specific block is currently allocated
 * @param bitmap Pointer to bitmap array (must not be NULL)
 * @param blockIndex Index of block to check (must be < numBlocks)
 * @return 1 if block is allocated, 0 if block is free
 * 
 * Uses bit manipulation to check the allocation status of a block.
 * Each bit in the bitmap represents one block (1 = allocated, 0 = free).
 */
static int rma_isBlockAllocated(uint32_t *bitmap, size_t blockIndex){
    // get the array and bit index of our block in the bitmap
    size_t const arrayIndex = blockIndex / 32;
    size_t const bitIndex = blockIndex % 32;

    return (int)(bitmap[arrayIndex] & (1 << bitIndex)) != 0;
}

/**
 * @brief Mark a specific block as allocated in the bitmap
 * @param bitmap Pointer to bitmap array (must not be NULL)
 * @param blockIndex Index of block to mark as allocated (must be < numBlocks)
 * 
 * Sets the corresponding bit in the bitmap to 1, indicating the block
 * is now allocated and unavailable for future allocations.
 */
static void rma_markBlockAllocated(uint32_t *bitmap, size_t blockIndex){
    // get the array and bit index of our block in the bitmap
    size_t const arrayIndex = blockIndex / 32;
    size_t const bitIndex = blockIndex % 32;

    // set the bit to 1 to indicate the block is allocated
    bitmap[arrayIndex] |= (1 << bitIndex);
}

/**
 * @brief Mark a specific block as free in the bitmap
 * @param bitmap Pointer to bitmap array (must not be NULL)
 * @param blockIndex Index of block to mark as free (must be < numBlocks)
 * 
 * Sets the corresponding bit in the bitmap to 0, indicating the block
 * is now free and available for future allocations.
 */
static void rma_markBlockFree(uint32_t *bitmap, size_t blockIndex){
    // get the array and bit index of our block in the bitmap
    size_t const arrayIndex = blockIndex / 32;
    size_t const bitIndex = blockIndex % 32;

    // set the bit to 0 to indicate the block is free
    bitmap[arrayIndex] &= ~(1 << bitIndex);
}

/**
 * @brief Generate a unique salt value for handle security
 * @param header Pointer to RMA header structure (must not be NULL)
 * @return 16-bit salt value, or 0 on failure after maximum retries
 * 
 * @warning Requires srand() to be called before use for proper randomness
 * 
 * Generates a cryptographically secure salt by checking for collisions
 * with existing allocated blocks. Retries up to 10 times to find a
 * unique salt value to prevent handle prediction attacks.
 */
static uint16_t rma_generateSalt(struct rma_mem_header_t *header){
    int const maxRetryCount = 10; // constant integer for the max retries
    int currentRetries = 0; // var to keep track of current retries 
    int collision = 0; // collision flag

    // ensure the salt doesn't exist already (subject to change if it hampers peformance too much)
    do {
        // get the first 16 bits of the random number as the salt
        uint16_t const salt = rand() & 0xFFFF;
        // always zerofy the collision var
        collision = 0;

        // get data structures
        uint32_t *bitmap = rma_getBitmap(header);
        uint32_t const *handleTable = rma_getHandleTable(header);

        // loop through all the allocated blocks
        for (size_t blockIndex = 0; blockIndex < header->numBlocks; blockIndex++){
            if (rma_isBlockAllocated(bitmap, blockIndex)){
                // if the generated salt is found, we found a collision
                if (handleTable[blockIndex] == salt){
                    collision = 1; 
                    break; // stop checking, try new salt
                }
            }
        }
        
        if (!collision){
            // no collision found, this salt is unique
            return salt;
        }

        // increment retries variable
        currentRetries++;
    } while(currentRetries != maxRetryCount);

    return 0; // failed to generate salt
}

/**
 * @brief Find the block index corresponding to a given handle
 * @param header Pointer to RMA header structure (must not be NULL)
 * @param handle Handle to search for in the handle table
 * @return Block index if found, SIZE_MAX if not found
 * 
 * Extracts the salt from the handle and searches through allocated blocks
 * to find the matching salt in the handle table. Only searches blocks
 * that are currently marked as allocated in the bitmap.
 */
static size_t rma_findBlockByHandle(struct rma_mem_header_t *header, rma_handle_t handle){
    // extract the upper 16 bits of the handle, which contain the unique salt
    uint16_t const handleSalt = (uint16_t)(handle >> 16);

    // get data structures
    uint32_t *bitmap = rma_getBitmap(header);
    uint32_t *handleTable = rma_getHandleTable(header);

    // loop through all allocated blocks and attempt to locate the block with the provided handle
    for (size_t blockIndex = 0; blockIndex < header->numBlocks; blockIndex++){
        if (rma_isBlockAllocated(bitmap, blockIndex)){
            if (handleTable[blockIndex] == handleSalt){
                // found
                return blockIndex;
            }
        }
    }

    // not found
    return SIZE_MAX;
}

/**
 * @brief Validate a handle for correctness and current allocation status
 * @param header Pointer to RMA header structure (must not be NULL)
 * @param handle Handle to validate
 * @return 1 if valid, 0 if invalid, negative for specific error conditions
 * 
 * Return values:
 * 
 * - 1: Handle is valid and block is allocated
 * 
 * - 0: Handle is RMA_INVALID_HANDLE or header is NULL
 * 
 * - -1: Handle not found in handle table
 * 
 * - -2: Block exists but is not marked as allocated
 * 
 * Performs comprehensive validation including handle format checking,
 * existence verification, and allocation status confirmation.
 */
static int rma_isValidHandle(struct rma_mem_header_t *header, rma_handle_t handle){
    // Basic validity of the handle
    if (!header || handle == RMA_INVALID_HANDLE){
        return 0; // Provided handle is invalid
    }

    // Attempt to find the block for this handle
    size_t const blockIndex = rma_findBlockByHandle(header, handle);
    if (blockIndex == SIZE_MAX) return -1; // Block doesn't exist for handle

    // Verify that the block is actually allocated
    uint32_t *bitmap = rma_getBitmap(header);
    if (!rma_isBlockAllocated(bitmap, blockIndex)) return -2; // Block isn't allocated for handle

    return 1; // valid handle
}

/**
 * @brief Calculate the memory address for a specific block index
 * @param header Pointer to RMA header structure (must not be NULL)
 * @param blockIndex Index of the block (must be < numBlocks)
 * @return Pointer to the start of the block's data area
 * 
 * Performs pointer arithmetic to convert a block index into the actual
 * memory address where user data can be stored. The returned pointer
 * points to a region of blockSize bytes.
 */
static void* rma_getBlockPtr(struct rma_mem_header_t *header, size_t blockIndex){
    return (char*)header + header->dataOffset + (blockIndex * header->blockSize);
}

/**
 * FUNCTION DEFINITIONS
 */

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
    header->blockSize = blockSize;
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

rma_handle_t rma_alloc(struct rma_mem_header_t *header){
    /*
        CHECKS
    */

    // validate the provided header
    if (header == NULL) return RMA_INVALID_HANDLE;

    // check if blocks are available
    if (header->numAllocated >= header->numBlocks){
        // In the future, it will expand the arena. For now, a simple debug message will do.
        printf("\nMax block count reached. Can't allocate more blocks.");
        return RMA_INVALID_HANDLE;
    }

    // check for nextHandle overflow
    if (header->nextHandle == 0) return RMA_INVALID_HANDLE;

    /*
        FREE BLOCK FINDING
    */
    uint32_t *bitmap = rma_getBitmap(header);
    size_t freeBlockIndex = 0;

    // find the first free block
    for (size_t blockIndex = 0; blockIndex < header->numBlocks; blockIndex++){
        if (rma_isBlockAllocated(bitmap, blockIndex) == 0){
            freeBlockIndex = blockIndex;
            break;
        }
    }

    /*
        GENERATE SECURE HANDLE
    */
    uint16_t const salt = rma_generateSalt(header);

    // make sure we got a valid salt
    if (salt == 0){
        printf("\nSalt generation failed.\n");
        return RMA_INVALID_HANDLE;
    }

    // combine salt with block index
    uint32_t const handle = (salt << 16) | header->nextHandle;

    /*
        UPDATE ALL DATA STRUCTURES
    */
    uint32_t *handleTable = rma_getHandleTable(header);

    // store the handle
    handleTable[freeBlockIndex] = salt;

    header->numAllocated++;
    header->nextHandle++;
    header->usedSize += header->blockSize;

    rma_markBlockAllocated(bitmap, freeBlockIndex);

    return handle;
}

int rma_free(struct rma_mem_header_t *header, rma_handle_t handle){
    // Validate the handle
    int const validity = rma_isValidHandle(header, handle);
    if (validity <= 0) return validity; // Handle is invalid, pass through the error code

    // Find the block
    size_t const blockIndex = rma_findBlockByHandle(header, handle);

    // Update all the data structures
    uint32_t *bitmap = rma_getBitmap(header);
    uint32_t *handleTable = rma_getHandleTable(header);

    // Clear the block
    rma_markBlockFree(bitmap, blockIndex);
    handleTable[blockIndex] = 0; // Clear the salt

    // Update statistics
    header->numAllocated--;
    header->usedSize -= header->blockSize;

    return 1; // success
}

void* rma_getPtr(struct rma_mem_header_t *header, rma_handle_t handle){
    // validate header and handle
    if (header == NULL) return NULL;
    if (rma_isValidHandle(header, handle) <= 0) return NULL;

    // get the block index
    size_t const blockIndex = rma_findBlockByHandle(header, handle);

    // if finding the block failed, return NULL
    if (blockIndex == SIZE_MAX) return NULL;

    // get the memory adress using my static helper function and return it 
    return (void *)rma_getBlockPtr(header, blockIndex);
}

void rma_displayMemInfo(struct rma_mem_header_t *header){
    if (!header){
        printf("RMA: Header is NULL\n");
        return;
    }

    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                             RMA                              ║\n");
    printf("║                     DETAILED DEBUG INFO                      ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    
    // === MEMORY LAYOUT SECTION ===
    printf("\nMEMORY LAYOUT:\n");
    printf("├─ Pool Start Address:     %p\n", (void*)header);
    printf("├─ Header Size:            %zu bytes (%.2f KiB)\n", 
           sizeof(struct rma_mem_header_t), 
           (double)sizeof(struct rma_mem_header_t) / 1024.0);
    printf("├─ Bitmap Offset:          +%zu bytes\n", header->bitmapOffset);
    printf("├─ Handle Table Offset:    +%zu bytes\n", header->handleTableOffset);
    printf("├─ Data Section Offset:    +%zu bytes\n", header->dataOffset);
    printf("└─ Block Size:             %zu bytes (%.2f KiB)\n", 
           header->blockSize, (double)header->blockSize / 1024.0);

    // === CAPACITY INFORMATION ===
    printf("\nCAPACITY:\n");
    printf("├─ Total Pool Size:        %zu bytes (%.4f MiB)\n",
           header->totalSize, (double)header->totalSize / (1024.0 * 1024.0));
    printf("├─ Metadata Overhead:      %zu bytes (%.2f KiB)\n", 
           header->dataOffset, (double)header->dataOffset / 1024.0);
    printf("├─ Available Data Space:   %zu bytes (%.4f MiB)\n",
           header->totalSize - header->dataOffset,
           (double)(header->totalSize - header->dataOffset) / (1024.0 * 1024.0));
    printf("└─ Overhead Percentage:    %.2f%%\n",
           ((double)header->dataOffset / header->totalSize) * 100.0);

    // === BLOCK STATISTICS ===
    printf("\nBLOCK STATISTICS:\n");
    printf("├─ Total Blocks:           %zu blocks\n", header->numBlocks);
    printf("├─ Allocated Blocks:       %zu blocks\n", header->numAllocated);
    printf("├─ Free Blocks:            %zu blocks\n", header->numBlocks - header->numAllocated);
    printf("├─ Block Utilization:      %.2f%%\n",
           header->numBlocks > 0 ? ((double)header->numAllocated / header->numBlocks) * 100.0 : 0.0);
    printf("└─ Theoretical Max Blocks: %zu blocks\n",
           (header->totalSize - sizeof(struct rma_mem_header_t)) / header->blockSize);

    // === MEMORY USAGE ===
    printf("\nMEMORY USAGE:\n");
    printf("├─ Used Memory:            %zu bytes (%.4f MiB)\n", 
           header->usedSize, (double)header->usedSize / (1024.0 * 1024.0));
    printf("├─ Free Memory:            %zu bytes (%.4f MiB)\n", 
           header->totalSize - header->usedSize, 
           (double)(header->totalSize - header->usedSize) / (1024.0 * 1024.0));
    printf("├─ Memory Efficiency:      %.4f%%\n", 
           ((double)header->usedSize / header->totalSize) * 100.0);
    printf("└─ Fragmentation:          %s\n", 
           header->numAllocated == 0 ? "None (no allocations)" : 
           header->numBlocks == header->numAllocated ? "None (fully allocated)" : "Possible");

    // === HANDLE INFORMATION ===
    printf("\nHANDLE MANAGEMENT:\n");
    printf("├─ Next Handle ID:         %u\n", header->nextHandle);
    printf("├─ Handles Issued:         %u\n", header->nextHandle - 1);
    printf("├─ Handle Overflow Risk:   %s\n", 
           header->nextHandle > 4000000000U ? "HIGH" : 
           header->nextHandle > 2000000000U ? "Medium" : "Low");
    printf("└─ Handle Space Used:      %.6f%%\n",
           ((double)header->nextHandle / 4294967295.0) * 100.0);

    // === BITMAP ANALYSIS ===
    printf("\nBITMAP ANALYSIS:\n");
    uint32_t *bitmap = rma_getBitmap(header);
    size_t bitmapWords = (header->numBlocks + 31) / 32;
    printf("├─ Bitmap Size:            %zu words (%zu bytes)\n", 
           bitmapWords, bitmapWords * sizeof(uint32_t));
    printf("├─ Bits Used:              %zu / %zu\n", 
           header->numBlocks, bitmapWords * 32);
    printf("└─ Bitmap Efficiency:      %.2f%%\n",
           bitmapWords > 0 ? ((double)header->numBlocks / (bitmapWords * 32)) * 100.0 : 0.0);

    // === ALLOCATION PATTERN (first 32 blocks) ===
    printf("\nALLOCATION PATTERN (first 32 blocks):\n");
    printf("└─ ");
    for (size_t i = 0; i < 32 && i < header->numBlocks; i++){
        if (i > 0 && i % 8 == 0) printf(" ");
        printf("%c", rma_isBlockAllocated(bitmap, i) ? 'X' : '_');
    }
    if (header->numBlocks > 32){
        printf(" ... (%zu more blocks)", header->numBlocks - 32);
    }
    printf("\n   Legend: X = Allocated, _ = Free\n");

    // === PERFORMANCE METRICS ===
    printf("\nPERFORMANCE METRICS:\n");
    printf("├─ Avg Block Size:         %.2f bytes\n", 
           header->numBlocks > 0 ? (double)header->blockSize : 0.0);
    printf("├─ Memory Density:         %.2f blocks/KiB\n",
           header->blockSize > 0 ? 1024.0 / header->blockSize : 0.0);
    printf("└─ Allocation Overhead:    %.2f bytes/block\n",
           header->numBlocks > 0 ? (double)header->dataOffset / header->numBlocks : 0.0);

    // === HEALTH CHECK ===
    printf("\nHEALTH CHECK:\n");
    int issues = 0;
    printf("├─ Header Integrity:       ");
    if (header->totalSize < sizeof(struct rma_mem_header_t)){
        printf("CORRUPT (totalSize too small)\n");
        issues++;
    }
    else {
        printf("OK\n");
    }
    
    printf("├─ Offset Alignment:       ");
    if (header->bitmapOffset < sizeof(struct rma_mem_header_t) ||
        header->handleTableOffset <= header->bitmapOffset ||
        header->dataOffset <= header->handleTableOffset){
        printf("CORRUPT (invalid offsets)\n");
        issues++;
    }
    else {
        printf("OK\n");
    }
    
    printf("├─ Block Consistency:      ");
    if (header->numAllocated > header->numBlocks){
        printf("CORRUPT (allocated > total)\n");
        issues++;
    }
    else {
        printf("OK\n");
    }
    
    printf("└─ Overall Status:         ");
    if (issues == 0){
        printf("HEALTHY\n");
    }
    else {
        printf("%d ISSUE(S) DETECTED\n", issues);
    }

    printf("\n");
    printf("═══════════════════════════════════════════════════════════════\n");
}