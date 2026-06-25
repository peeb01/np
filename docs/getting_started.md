# Getting Started with NP Language

NP is a statically/dynamically typed scripting language that transpiles directly to C++17 and compiles into highly optimized native machine code. Memory is automatically managed via the Boehm Garbage Collector.

---

## Installation & Setup

### 1. Prerequisites (Host Machine)
To build and run the NP compiler directly on your host machine, you need:
*   **Operating System:** Linux/WSL or macOS.
*   **C++ Compiler:** `g++` (GCC) with support for C++17.
*   **Garbage Collector:** Boehm GC (`libgc`).

On Ubuntu/Debian/WSL, install them using:
```bash
sudo apt-get update
sudo apt-get install -y build-essential libgc-dev
```

On Alpine Linux:
```bash
apk add build-essential boehm-gc-dev
```

On macOS:
```bash
brew install bdw-gc gcc
```

### 2. Building the Compiler
Clone the repository and run `make`:
```bash
make re
```
This produces the native compiler executable `np`.

---

## Running NP Code

The NP compiler supports two modes of execution:

### 1. Scripting/Run Mode (Immediate Execution)
Runs an NP file immediately, executing it like a script. It compiles the source file to a temporary binary, runs it, and automatically deletes it.
```bash
./np tests/advanced_features_test.np
```

### 2. Compilation/Build Mode (Ahead-Of-Time)
Compiles your NP source code directly into a standalone, highly optimized executable file (default is `app.out`).
```bash
./np build tests/advanced_features_test.np
# Run the generated binary
./app.out
```

---

## Quick Start with Docker

If you prefer not to install anything on your host machine, you can run the compiler using Docker.

Set up an alias in your shell:
```bash
alias np='docker run --rm -it -v "$PWD":/workspace pib21/np-lang:alpine-3.22'
```

Now run or build your files using Docker:
```bash
# Run a file
np tests/advanced_features_test.np

# Build a binary
np build tests/advanced_features_test.np
```
