# Brainfuck Compiler (bfc)

A professional Brainfuck compiler written in C that compiles Brainfuck code to x86-64 assembly language.

## Features

- ✅ **Full Brainfuck support** - All 8 operations: `+ - > < . , [ ]`
- ✅ **Optimizations** - Combines repeated operations (e.g., `++++` → `add $4`)
- ✅ **Error checking** - Detects unmatched brackets
- ✅ **Native code generation** - Produces x86-64 assembly (AT&T syntax)
- ✅ **30,000 bytes of memory** - Standard Brainfuck memory size
- ✅ **Professional code** - Clean, well-documented C code

## Brainfuck Language Reference

| Command | Description |
|---------|-------------|
| `+` | Increment the byte at the data pointer |
| `-` | Decrement the byte at the data pointer |
| `>` | Move the data pointer right |
| `<` | Move the data pointer left |
| `.` | Output the byte at the data pointer |
| `,` | Input one byte and store at the data pointer |
| `[` | Jump forward past matching `]` if byte at pointer is 0 |
| `]` | Jump back to matching `[` if byte at pointer is non-zero |

## Compilation

Build the compiler:
```bash
make
```

Or manually:
```bash
gcc -Wall -Wextra -O2 -o bfc bfc.c
```

## Usage

Compile a Brainfuck program:
```bash
./bfc program.bf output.s
```

Assemble and link:
```bash
as output.s -o output.o
ld output.o -o program
```

Run:
```bash
./program
```

### All-in-one script

Create a bash script `bf_run.sh`:
```bash
#!/bin/bash
./bfc $1 temp.s && as temp.s -o temp.o && ld temp.o -o temp_prog && ./temp_prog
rm -f temp.s temp.o temp_prog
```

Then:
```bash
chmod +x bf_run.sh
./bf_run.sh examples/hello.bf
```

## Examples

### Hello World
```bash
./bfc examples/hello.bf hello.s
as hello.s -o hello.o
ld hello.o -o hello
./hello
```
Output: `Hello World!`

### Simple Program
```bash
./bfc examples/simple.bf simple.s
as simple.s -o simple.o
ld simple.o -o simple
./simple
```
Output: `F` (ASCII 70)

### ROT13 Cipher
```bash
./bfc examples/rot13.bf rot13.s
as rot13.s -o rot13.o
ld rot13.o -o rot13
echo "Hello" | ./rot13
```
Output: `Uryyb`

## How It Works

### 1. Lexical Analysis
The compiler reads the source file and filters out non-Brainfuck characters (comments are ignored).

### 2. Optimization
Repeated operations are combined:
- `++++` → `addb $4, (%r12)`
- `>>>>` → `addq $4, %r12`

### 3. Code Generation
Each Brainfuck instruction is translated to x86-64 assembly:
- Uses register `%r12` as the data pointer
- Memory array is in the `.data` section
- Uses Linux syscalls for I/O (`sys_read`, `sys_write`)

### 4. Loop Handling
- `[` generates a label and conditional jump
- `]` jumps back to the matching label
- Stack tracks nested loops

## Memory Layout

```
Register Usage:
  %r12  - Data pointer (points to current cell in memory array)
  %rax  - Syscall number / temporary
  %rdi  - Syscall arg 1
  %rsi  - Syscall arg 2
  %rdx  - Syscall arg 3

Data Section:
  memory: .zero 30000   (30KB byte array)
```

## Generated Assembly Example

For Brainfuck code `++>++`:
```asm
    .section .data
memory:
    .zero 30000

    .section .text
    .globl _start

_start:
    leaq memory(%rip), %r12
    addb $2, (%r12)      # ++ (optimized)
    incq %r12            # >
    addb $2, (%r12)      # ++ (optimized)
    
    movq $60, %rax       # sys_exit
    xorq %rdi, %rdi
    syscall
```

## Requirements

- **Compiler**: GCC or Clang
- **Assembler**: GNU `as` (part of binutils)
- **Linker**: GNU `ld` (part of binutils)
- **Platform**: Linux x86-64

## Architecture

```
Source Code (.bf)
       ↓
   [Lexer] - Read and filter characters
       ↓
   [Parser] - Validate brackets
       ↓
   [Optimizer] - Combine repeated ops
       ↓
   [Code Generator] - Emit assembly
       ↓
Assembly Code (.s)
       ↓
   [Assembler (as)]
       ↓
Object Code (.o)
       ↓
   [Linker (ld)]
       ↓
Executable Binary
```

## Advanced Features

### Optimizations Implemented
1. **Run-length encoding** - Multiple identical operations combined
2. **Direct value modifications** - `add/sub` instead of multiple `inc/dec`

### Potential Extensions
- Compile to other architectures (ARM, RISC-V)
- JIT compilation
- More aggressive optimizations (clear loops, copy loops)
- Bytecode generation for a VM
- LLVM IR backend

## Error Handling

The compiler detects:
- ✅ Unmatched `[` brackets
- ✅ Unmatched `]` brackets
- ✅ File I/O errors
- ✅ Memory allocation failures

## Performance

The generated code is fairly efficient:
- Direct memory access (no bounds checking for speed)
- Optimized repeated operations
- Native x86-64 instructions

## License

Free to use, modify, and distribute.

## Contributing

Want to improve the compiler? Ideas:
- Add more optimizations
- Support for debugging symbols
- Colored error messages
- Interactive mode
- Cross-platform support (Windows, macOS)

---

**Created as a demonstration of compiler construction in C**

Enjoy compiling! 🧠🔥
# brainF-compiler-
