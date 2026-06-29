# Getting Started with NP Language

NP is a statically/dynamically typed scripting language that compiles directly to highly optimized native machine-code binaries using the **LLVM C++ API** and a static C-compatible runtime library (`runtime/libnpruntime.a`). Memory is automatically managed via Reference Counting (RAII) using C++ smart pointers underneath.

---

## Installation & Setup

We distinguish between **Language Users** (who just want to write and run `.np` programs) and **Compiler Developers** (who want to modify and compile the `np` compiler C++ source code itself).

### 1. For Language Users (Writing NP Code)
If you download a pre-built release binary of the `np` compiler:
*   **No LLVM required:** You **DO NOT** need to install `llvm-dev` or any LLVM libraries on your machine.
*   **Requirements:** You only need a standard C++ linker (like `g++` or `clang` from GCC/Clang) installed to link the final executable.
    - **Ubuntu/Debian/WSL:** `sudo apt-get install -y build-essential`
    - **macOS:** Xcode Command Line Tools (`xcode-select --install`)
    - **Windows:** MinGW-w64 (already installed if you have GCC/g++ in your PATH)

### 2. For Compiler Developers (Building the Compiler from Source)
If you are developing the compiler itself and compiling `core/*.cpp` and `main.cpp`:
*   **Requirements:** You need the LLVM development headers and libraries (LLVM 18 recommended) and a C++17 compiler.
    - **Ubuntu/Debian/WSL:** `sudo apt-get install -y build-essential llvm-dev`
    - **Alpine Linux:** `apk add build-base llvm-dev llvm-static`
    - **macOS:** `brew install llvm gcc`

To build the compiler from source:
```bash
make re
```
This compiles the C++ codebase and generates the native executable `./np` in the workspace root.

---

## Running NP Code

The NP compiler supports two modes of execution:

### 1. Scripting/Run Mode (Immediate Execution)
Compiles your NP source file into a temporary binary, executes it immediately on your system, and cleans up the temporary files automatically.
```bash
./np tests/basic.np
```

### 2. Compilation/Build Mode (Ahead-Of-Time)
Compiles your NP source code directly into a standalone, highly optimized native executable binary (default is `app.out`).
```bash
./np build tests/basic.np
# Run the generated binary
./app.out
```

---

## Quick Start with Docker (Zero Dependencies)

If you prefer not to install anything on your host machine (neither LLVM nor GCC), you can run the compiler using Docker.

Set up an alias in your shell:
```bash
alias np='docker run --rm -it -v "$PWD":/workspace pib21/np-lang:alpine-3.22'
```

Now run or build your files using Docker:
```bash
# Run a file immediately
np tests/basic.np

# Build a binary
np build tests/basic.np
```
