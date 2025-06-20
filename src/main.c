/**
 * @file main.c
 * @brief RMA memory allocator test program and usage examples
 * @author Robkoo
 * @date 19.06.2025
 * @version 0.0.2
 * @since 0.0.1
 * 
 * Contains test cases and demonstration of RMA (Robkoo's Memory Allocator)
 * functionality. Provides examples of memory pool initialization, allocation
 * testing, and proper cleanup procedures.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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
    // Seed the random number generator
    srand(time(NULL));

    // Initialize memory pool
    struct rma_mem_header_t *allocator = rma_memHeaderInit(STARTING_ARENA_SIZE, DEFAULT_BLOCK_SIZE);
    
    if (!allocator){
        printf("Failed to initialize RMA!\n");
        return 1;
    }
    
    printf("RMA initialized successfully!\n");

    // ========================================
    // Test 1: Basic Functionality Test
    // ========================================
    printf("\n=== Test 1: Basic Functionality ===\n");

    // Allocate a block
    rma_handle_t handle = rma_alloc(allocator);
    printf("Allocated handle: 0x%08X\n", handle);

    // Get pointer to the memory
    char *ptr = (char*)rma_getPtr(allocator, handle);
    if (ptr == NULL){
        printf("[ERR] rma_getPtr returned NULL for valid handle!\n");
    }
    else {
        printf("[SUCCESS] Got valid pointer: %p\n", (void*)ptr);
        
        // Write some data
        strcpy(ptr, "Hello RMA!");
        printf("[SUCCESS] Wrote data: '%s'\n", ptr);
        
        // Read it back
        printf("[SUCCESS] Read back: '%s'\n", ptr);
    }

    // ========================================
    // Test 2: Invalid Handle Test
    // ========================================
    printf("\n=== Test 2: Invalid Handles ===\n");

    // Test with invalid handle
    char *invalid_ptr = (char*)rma_getPtr(allocator, RMA_INVALID_HANDLE);
    if (invalid_ptr == NULL){
        printf("[SUCCESS] Correctly returned NULL for RMA_INVALID_HANDLE\n");
    }
    else {
        printf("[ERR] Should return NULL for invalid handle!\n");
    }

    // Test with freed handle
    rma_free(allocator, handle);
    char *freed_ptr = (char*)rma_getPtr(allocator, handle);
    if (freed_ptr == NULL){
        printf("[SUCCESS] Correctly returned NULL for freed handle\n");
    }
    else {
        printf("[ERR] Should return NULL for freed handle!\n");
    }

    // Test with completely fake handle
    rma_handle_t fake_handle = 0x12345678;
    char *fake_ptr = (char*)rma_getPtr(allocator, fake_handle);
    if (fake_ptr == NULL){
        printf("[SUCCESS] Correctly returned NULL for fake handle\n");
    }
    else {
        printf("[ERR] Should return NULL for fake handle!\n");
    }

    // ========================================
    // Test 3: Multiple Blocks Test
    // ========================================
    printf("\n=== Test 3: Multiple Blocks ===\n");

    // Allocate multiple blocks
    rma_handle_t h1 = rma_alloc(allocator);
    rma_handle_t h2 = rma_alloc(allocator);
    rma_handle_t h3 = rma_alloc(allocator);

    printf("Allocated handles: 0x%08X, 0x%08X, 0x%08X\n", h1, h2, h3);

    // Get pointers and write different data
    int *ptr1 = (int*)rma_getPtr(allocator, h1);
    int *ptr2 = (int*)rma_getPtr(allocator, h2);
    int *ptr3 = (int*)rma_getPtr(allocator, h3);

    if (ptr1 && ptr2 && ptr3){
        *ptr1 = 1111;
        *ptr2 = 2222;
        *ptr3 = 3333;
        
        printf("Block 1 contains: %d (at %p)\n", *ptr1, (void*)ptr1);
        printf("Block 2 contains: %d (at %p)\n", *ptr2, (void*)ptr2);
        printf("Block 3 contains: %d (at %p)\n", *ptr3, (void*)ptr3);
        
        // Verify they're different addresses
        printf("Addresses are different: %s\n", 
               (ptr1 != ptr2 && ptr2 != ptr3 && ptr1 != ptr3) ? "[SUCCESS] YES" : "[ERR] NO");
        
        // Verify data integrity
        if (*ptr1 == 1111 && *ptr2 == 2222 && *ptr3 == 3333){
            printf("[SUCCESS] Data integrity maintained across blocks\n");
        }
        else {
            printf("[ERR] Data corruption detected!\n");
        }
    }
    else {
        printf("[ERR] One or more pointers are NULL!\n");
    }

    // ========================================
    // Test 4: Memory Boundaries Test
    // ========================================
    printf("\n=== Test 4: Memory Boundaries ===\n");

    rma_handle_t boundary_handle = rma_alloc(allocator);
    char *boundary_ptr = (char*)rma_getPtr(allocator, boundary_handle);

    if (boundary_ptr){
        printf("Testing full block write/read (%d bytes)...\n", DEFAULT_BLOCK_SIZE);
        
        // Fill the entire block with a pattern
        for (size_t i = 0; i < DEFAULT_BLOCK_SIZE; i++){
            boundary_ptr[i] = (char)(i % 256);
        }
        
        // Verify the pattern
        int errors = 0;
        for (size_t i = 0; i < DEFAULT_BLOCK_SIZE; i++){
            if (boundary_ptr[i] != (char)(i % 256)){
                errors++;
            }
        }
        
        if (errors == 0){
            printf("[SUCCESS] Block boundaries test passed - wrote/read %d bytes successfully\n", DEFAULT_BLOCK_SIZE);
            printf("   First byte: %d, Middle byte: %d, Last byte: %d\n", 
                   (unsigned char)boundary_ptr[0], 
                   (unsigned char)boundary_ptr[511], 
                   (unsigned char)boundary_ptr[1023]);
        }
        else {
            printf("[ERR] Block boundaries test failed - %d errors detected\n", errors);
        }
    }
    else {
        printf("[ERR] Failed to get pointer for boundary test\n");
    }

    // ========================================
    // Test 5: Fragmentation & Reuse Test
    // ========================================
    printf("\n=== Test 5: Fragmentation & Reuse ===\n");

    // Free the middle block to create fragmentation
    printf("Freeing middle block (h2)...\n");
    rma_free(allocator, h2);
    
    // Allocate a new block - should reuse the freed slot
    rma_handle_t h4 = rma_alloc(allocator);
    char *ptr4 = (char*)rma_getPtr(allocator, h4);
    
    if (ptr4){
        strcpy(ptr4, "Reused block!");
        printf("[SUCCESS] Successfully reused freed block: '%s'\n", ptr4);
        
        // Check if it reused the same memory location as h2
        if (ptr4 == (char*)ptr2){
            printf("[SUCCESS] Correctly reused the same memory location\n");
        }
        else {
            printf("[SUCCESS?] Used different memory location (also valid)\n");
        }
    }
    else {
        printf("[ERR] Failed to allocate after fragmentation\n");
    }

    // ========================================
    // Final Memory State
    // ========================================
    printf("\n=== Final Memory State ===\n");
    rma_displayMemInfo(allocator);

    // Cleanup
    free(allocator);
    printf("\n[SUCCESS] All tests completed!\n");
    return 0;
}