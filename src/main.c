/**
 * @file main.c
 * @brief RMA memory allocator test program and usage examples
 * @author Robkoo
 * @date 19.06.2025
 * @version 0.0.1
 * @since 0.0.1
 * 
 * Contains test cases and demonstration of RMA (Robkoo's Memory Allocator)
 * functionality. Provides examples of memory pool initialization, allocation
 * testing, and proper cleanup procedures.
 */

#include <stdio.h>
#include <stdlib.h>
#include "memHeader.h"

// THIS PROJECT'S IDENTIFIER IS `RMA` - Robkoo's Memory Allocator.
// IT **WILL** BE PUT IN FRONT OF ALL FUNCTIONS, DEFINITIONS AND CUSTOM TYPES FOR CLARITY

/**
 * @brief Default memory arena size for testing
 * 
 * Initial memory pool size of 1 MiB provides sufficient space for
 * testing allocation patterns while remaining manageable for debugging.
 */
#define STARTING_ARENA_SIZE (1024 * 1024) // 1 MiB

/**
 * @brief Default block size for memory allocations
 * 
 * 1 KiB blocks provide a good balance between granularity and
 * metadata overhead for typical application usage patterns.
 */
#define DEFAULT_BLOCK_SIZE (1024) // 1KiB blocks

/**
 * @brief Main test function for RMA memory allocator
 * @return 0 on success, 1 on failure
 * 
 * Demonstrates basic RMA functionality including:
 * - Memory pool initialization
 * - Error handling for initialization failures
 * - Memory statistics display
 * - Proper cleanup and resource management
 */
int main(void){
    // Initialize memory pool
    struct rma_mem_header_t *allocator = rma_memHeaderInit(STARTING_ARENA_SIZE, DEFAULT_BLOCK_SIZE);
    
    if (!allocator){
        printf("Failed to initialize RMA!\n");
        return 1;
    }
    
    printf("RMA initialized successfully!\n\n");
    
    // Display initial state
    rma_displayMemInfo(allocator);
    
    // Clean up (for now just free the whole pool)
    free(allocator);
    
    return 0;
}