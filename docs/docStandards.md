# RMA Documentation Standards

## Overview

This document defines the documentation standards for the RMA (Robkoo's Memory Allocator) project. All code must follow these conventions to ensure maintainability and clarity.

## File Header Documentation

Every source file (`.c` and `.h`) must include a header comment:

```c
/**
 * @file filename.c
 * @brief Brief description of file purpose
 * @author Robkoo
 * @date DD.MM.YYYY
 * @version X.Y.Z
 * 
 * Detailed description of what this file contains and its role
 * in the RMA memory allocator system.
 */
```

### Example:
```c
/**
 * @file memHeader.c
 * @brief Core memory allocator implementation
 * @author Robkoo
 * @date 19.06.2025
 * @version 0.0.1
 * 
 * Contains the main memory pool initialization and management functions
 * for the RMA handle-based memory allocator. Handles bitmap tracking,
 * offset calculations, and memory pool setup.
 */
```

## Function Documentation

All functions are documented in the **header** file.
All functions must be documented using Doxygen-style comments:

```c
/**
 * @brief Brief description of function purpose
 * @param paramName Description of parameter and its constraints
 * @param paramName Description of parameter and its constraints
 * @return Description of return value and possible error conditions
 * 
 * @note Optional: Important notes about usage or behavior
 * @warning Optional: Critical warnings about potential issues
 * @see Optional: References to related functions
 * 
 * Detailed description of function behavior, algorithm used,
 * and any side effects or special considerations.
 */
```

### Example:
```c
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
```

## Structure Documentation

All structures must be documented with overall purpose and each member:

```c
/**
 * @brief Brief description of structure purpose
 * 
 * Detailed description of what this structure represents,
 * how it's used, and any important relationships.
 */
struct structure_name {
    /**< Brief description of member */
    type member_name;
    
    /**< Brief description of member with constraints/units */
    type member_name;
};
```

### Example:
```c
/**
 * @brief Main header structure containing all memory pool metadata
 * 
 * This structure sits at the beginning of every RMA memory pool and
 * contains all necessary information for managing allocations, tracking
 * usage, and converting handles to memory addresses.
 */
struct rma_mem_header_t {
    size_t totalSize;        /**< Total pool size in bytes */
    size_t numBlocks;        /**< Number of allocatable blocks */
    size_t numAllocated;     /**< Currently allocated blocks count */
    uint32_t nextHandle;     /**< Next handle ID to assign (starts at 1) */
    
    size_t bitmapOffset;     /**< Byte offset from pool start to bitmap */
    size_t handleTableOffset; /**< Byte offset from pool start to handle table */
    size_t dataOffset;       /**< Byte offset from pool start to first block */
};
```

## Typedef Documentation

```c
/**
 * @brief Brief description of typedef purpose
 * 
 * Explanation of what this type represents and why it exists.
 */
typedef existing_type new_type_name;
```

### Example:
```c
/**
 * @brief Handle type for safe memory references
 * 
 * Handles are used instead of raw pointers to provide memory safety
 * and allow for memory defragmentation without invalidating references.
 * A handle value of 0 (RMA_INVALID_HANDLE) indicates an invalid handle.
 */
typedef uint32_t rma_handle_t;
```

## Macro Documentation

```c
/**
 * @brief Brief description of macro purpose
 * @param param Description of macro parameter
 * @return Description of what macro expands to
 * 
 * Detailed explanation of macro behavior and usage.
 */
#define MACRO_NAME(param) (expansion)
```

### Example:
```c
/**
 * @brief Convert bitmap offset to actual pointer
 * @param header Pointer to RMA header structure
 * @return uint32_t* pointer to bitmap array
 * 
 * Performs pointer arithmetic to convert the stored bitmap offset
 * into a usable pointer for bitmap operations. The result points
 * to an array of uint32_t values for bit manipulation.
 */
#define RMA_GET_BITMAP(header) \
    ((uint32_t*)((char*)(header) + (header)->bitmapOffset))
```

## Constants Documentation

```c
/**
 * @brief Brief description of constant purpose
 * 
 * Explanation of what this constant represents and its usage.
 */
#define CONSTANT_NAME value
```

### Example:
```c
/**
 * @brief Invalid handle value indicating unallocated or freed memory
 * 
 * This value is returned by allocation functions when they fail,
 * and should never be passed to functions expecting valid handles.
 */
#define RMA_INVALID_HANDLE 0
```

## General Guidelines

### Mandatory Documentation
- **All public functions** must have complete documentation
- **All structures and their members** must be documented
- **All header files** must have file headers
- **All source files** must have file headers

### Optional but Recommended
- Internal/static functions should have brief comments
- Complex algorithms should have inline comments
- Magic numbers should be replaced with documented constants

### Style Guidelines
- Use present tense ("Returns pointer" not "Will return pointer")
- Be specific about parameter constraints and error conditions
- Include units for numerical parameters (bytes, milliseconds, etc.)
- Document memory ownership (who allocates, who frees)
- Mention thread safety if relevant

### Documentation Tools
- Use Doxygen-compatible comments (`/**` and `*/`)
- Tags to use: `@brief`, `@param`, `@return`, `@note`, `@warning`, `@see`
- Generate docs with: `doxygen` (configuration to be added later)

## Example: Fully Documented Function

```c
/**
 * @file memHeader.h
 * @brief Public API for RMA memory allocator
 * @author Robkoo
 * @date 19.06.2025
 * @version 0.0.1
 * 
 * Defines the public interface for the RMA (Robkoo's Memory Allocator)
 * handle-based memory management system. Provides functions for pool
 * initialization, block allocation, and memory debugging.
 */

/**
 * @brief Allocate a memory block and return its handle
 * @param header Pointer to initialized RMA header (must not be NULL)
 * @return Handle to allocated block, or RMA_INVALID_HANDLE on failure
 * 
 * @note Handles remain valid until explicitly freed with rma_free()
 * @warning Do not use handles after calling rma_free() on them
 * @see rma_free, rma_getPtr, rma_memHeaderInit
 * 
 * Searches the bitmap for the first available free block, marks it as
 * allocated, assigns it a unique handle, and updates the handle table.
 * The allocation process is O(n) in worst case where n is number of blocks.
 * 
 * Allocation fails if:
 * - header is NULL
 * - No free blocks available
 * - Handle ID overflow (after 4 billion allocations)
 */
rma_handle_t rma_alloc(struct rma_mem_header_t *header);
```

Following these standards ensures that the RMA codebase remains maintainable and professional as it grows in complexity.