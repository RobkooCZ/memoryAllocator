# RMA Changelog

## Template for each version entry:

### [VERSION X.Y.Z] - DD.MM.YYYY

#### Added
- Brief description of new features or additions.

#### Changed
- Brief description of changes or improvements.

#### Fixed
- Brief description of bug fixes.

#### Removed
- Brief description of removals or deprecations.

---

## Alpha Versions

### [VERSION 0.0.2] - 21.06.2025

#### Added
- static helper functions inside `memHeader.c`:
    - `rma_getBitmap()` - Get pointer to bitmap array for bit manipulation operations
    - `rma_getHandleTable()` - Get pointer to handle table array for handle-to-block mapping
    - `rma_isBlockAllocated()` - Check if a specific block is currently allocated
    - `rma_markBlockAllocated()` - Mark a specific block as allocated in the bitmap
    - `rma_markBlockFree()` - Mark a specific block as free in the bitmap
    - `rma_generateSalt()` - Generate a unique salt value for handle security
    - `rma_findBlockByHandle()` - Find the block index corresponding to a given handle
    - `rma_isValidHandle()` - Validate a handle for correctness and current allocation status
    - `rma_getBlockPtr()` - Calculate the memory address for a specific block index
- `rma_alloc()` function to allocate blocks
- block deallocation using `rma_free()`
- validation for handles, headers and functions
- `rma_generateSalt()` ensures security for handles to prevent prediction attacks
- validation system using `rma_isValidHandle()`
- comprehensive testing in `main()` to showcase the functionality and success of the project so far
- documentation for all functions added

#### Changed
- improved the `rma_displayMemInfo()` to showcase way more info in a nicer way for clear debugging

### [VERSION 0.0.1] - 19.06.2025

#### Added
- Core `rma_mem_header_t` custom type with layout offsets
- `rma_memHeaderInit()` function for memory pool initialization
- `rma_displayMemInfo()` for debugging and memory statistics
- Handle-based architecture foundation (`rma_handle_t` typedef)
- Bitmap allocation tracking structure
- Build system with Makefile and proper project structure
- Basic test program in `main.c`

### [VERSION 0.0.0] - 18.06.2025

#### Added
- Initial commit