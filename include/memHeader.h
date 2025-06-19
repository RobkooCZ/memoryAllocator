/**
 * @file memHeader.h
 * @brief Public API for RMA memory allocator
 * @author Robkoo
 * @date 19.06.2025
 * @version 0.0.1
 * @since 0.0.1
 * 
 * Defines the public interface for the RMA (Robkoo's Memory Allocator)
 * handle-based memory management system. Provides functions for pool
 * initialization, block allocation, and memory debugging.
*/

#ifndef MEM_HEADER
#define MEM_HEADER

#include <stdlib.h>
#include <stdint.h>

/**
 * @brief Handle type for safe memory references
 * 
 * Handles are used instead of raw pointers to provide memory safety
 * and allow for memory defragmentation without invalidating references.
 * A handle value of 0 (RMA_INVALID_HANDLE) indicates an invalid handle.
 */
typedef uint32_t rma_handle_t;

/**
 * @brief Invalid handle value indicating unallocated or freed memory
 * 
 * This value is returned by allocation functions when they fail,
 * and should never be passed to functions expecting valid handles.
 */
#define RMA_INVALID_HANDLE 0

/**
 * @brief Main header structure containing all memory pool metadata
 * 
 * This structure sits at the beginning of every RMA memory pool and
 * contains all necessary information for managing allocations, tracking
 * usage, and converting handles to memory addresses.
 */
struct rma_mem_header_t {
    size_t totalSize;        /**< Total pool size in bytes */
    size_t usedSize;         /**< Currently used bytes (including metadata) */
    
    size_t numBlocks;        /**< Number of allocatable blocks in pool */
    size_t numAllocated;     /**< Currently allocated blocks count */
    
    uint32_t nextHandle;     /**< Next handle ID to assign (starts at 1) */

    size_t bitmapOffset;     /**< Byte offset from pool start to bitmap */
    size_t handleTableOffset; /**< Byte offset from pool start to handle table */
    size_t dataOffset;       /**< Byte offset from pool start to first block */
};

/**
 * @brief Initialize a new RMA memory pool with specified parameters
 * @param totalSize Total size in bytes for the memory pool (must be > 1KB)
 * @param blockSize Size in bytes for each individual block (must be > 0)
 * @return Pointer to initialized header structure, or NULL on failure
 * 
 * @note The actual number of blocks may be less than totalSize/blockSize
 *       due to metadata overhead (bitmap, handle table, header)
 * @warning Caller is responsible for calling free() on the returned pointer
 * @see rma_displayMemInfo, rma_alloc, rma_free
 * 
 * Creates a single large memory allocation and subdivides it into:
 * - Header structure (metadata)
 * - Bitmap for tracking allocated/free blocks
 * - Handle table for handle-to-block mapping
 * - Actual data blocks for user allocation
 */
void* rma_memHeaderInit(size_t totalSize, size_t blockSize);

/**
 * @todo Implement
 */
rma_handle_t rma_alloc(struct rma_mem_header_t *header);

/**
 * @todo Implement
 */
void rma_free(struct rma_mem_header_t *header, rma_handle_t handle);

/**
 * @todo Implement
 */
void* rma_getPtr(struct rma_mem_header_t *header, rma_handle_t handle);

/**
 * @brief Display comprehensive memory pool statistics
 * @param header Pointer to initialized RMA header (must not be NULL)
 * 
 * @note Output is formatted for human readability
 * @see rma_memHeaderInit
 * 
 * Prints detailed information about memory usage including:
 * - Total/used/free memory in bytes and MiB
 * - Block allocation statistics
 * - Handle information
 * - Memory usage percentage
 */
void rma_displayMemInfo(struct rma_mem_header_t *header);

#endif // MEM_HEADER