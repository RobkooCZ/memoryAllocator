# Custom C memory allocator

## How to build

### Prerequisites
- GCC compiler with C23 support
- Make utility
- Linux/Unix environment

### Compilation

```bash
# Build the project
make

# Run the test program
./build/rma

# Clean build artifacts
make clean
```

The Makefile automatically:
- Creates the `build/` directory if it doesn't exist
- Compiles with `-Wall -Wextra -std=c23 -g` flags
- Links all source files into `build/rma` executable