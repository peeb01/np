# 🚀 NP Compiler Architecture

Welcome to the **NP Compiler** project!

This document explains the architecture and underlying processes of the compiler—how your `.np` code is translated into a blazing-fast executable binary.

---

## ⚙️ System Requirements (Local Installation)
If you are running the `np` binary directly on your host machine without Docker, your system **must** have a C++ compiler and the Boehm GC library installed:
*   **Compiler:** `g++` (GCC) with C++17 support.
*   **Library:** `libgc-dev` (Debian/Ubuntu), `boehm-gc-dev` (Alpine), or `bdw-gc` (macOS).

---

## ⚡ Quick Start with Docker (Run seamlessly)

You can run the compiler via Docker as if it were installed locally on your machine. Set up a shell alias to mount your current directory (`$PWD`) to the container's `/workspace`:

**Linux / macOS (Bash/Zsh):**
```bash
alias np='docker run --rm -it -v "$PWD":/workspace pib21/np-lang:alpine-3.22'

# Now you can just use it like a native compiler!
np tests/array.np
```
Or run with make

```bash
make run file-name.np
```

---
## 🏗️ Translation Pipeline (How it works)

The `np-lang` compiler acts as a **Transpiler (Source-to-Source Compiler)**. It does not generate assembly directly. Instead, it translates `.np` code into Modern C++ (C++17) and utilizes `g++` (GCC) as the backend to compile it into highly optimized native machine code.

### 📊 Compiler Pipeline Block Diagram

```text
   [ User Code ]
 +-----------------------+
 |   Source Code (.np)   |   e.g., main.np
 +-----------+-----------+
             |
             v
 +-----------+-----------+
 |     Lexer Phase       |   Scans raw text and splits it into Tokens.
 |  (Lexical Analysis)   |   (e.g., KEYWORD_FN, IDENTIFIER, INT)
 +-----------+-----------+
             | (Tokens Stream)
             v
 +-----------+-----------+
 |    Parser Phase       |   Syntax checking & Single-Pass
 | & Transpilation (C++) |   Direct Transpilation to C++ Strings.
 +-----------+-----------+
             | (C++ Code String)
             v
 +-----------+-----------+
 |    CodeGen Phase      |   Injects np-lang standard library and
 | (Code Generation)     |   writes to a temporary .cpp file.
 +-----------+-----------+
             | (output_tmp.cpp)
             v
 +-----------+-----------+
 |    C++ Compiler       |   Calls `g++ -O3 -std=c++17` to compile
 |    (GCC / g++)        |   the C++ code into Native Machine Code.
 +-----------+-----------+
             |
             v
 +-----------+-----------+
 |   Executable Binary   |   Final ready-to-run binary (app.out)
 +-----------------------+
```

---

##  Deep Dive: Core Mechanisms

### 1. How the Parser Works
The compiler uses a **Hand-written Recursive Descent Parser**. 
Unlike traditional compilers that build a massive Abstract Syntax Tree (AST) in memory before generating code, the `np-lang` parser does **Single-Pass Transpilation**:
*   **Simultaneous Analysis & Generation:** As it verifies the syntax rules (e.g., ensuring `if` statements end with `:`), it immediately constructs the equivalent C++ string.
*   **Context Separation:** The parsed code is dynamically split into `global_code` (for functions via `fn`) and `translated_code` (for statements that belong inside the `int main()` entry point).

### 2. Memory Management & Garbage Collection
`np-lang` handles memory management automatically, so you don't have to worry about manual memory allocation or memory leaks.
*   **Primitive Types:** Types like `int`, `float`, `bool`, and `string` are mapped directly to C++ primitives and `std::string`. These are allocated on the **Stack**, making them extremely fast and automatically cleaned up when they go out of scope.
*   **Complex Types (Arrays & Dictionaries):** `np-lang` integrates the **Boehm-Demers-Weiser Garbage Collector (Boehm GC)** for its heap-allocated structures. This allows the creation of complex, nested, or dynamically resizing arrays and dictionaries without memory leaks or the circular reference issues commonly found in ARC. The GC runs transparently in the background, automatically reclaiming unused memory during execution.

### 3. Does it use JIT (Just-In-Time) Compilation?
**No. np-lang is 100% Ahead-Of-Time (AOT) compiled.** 
While it feels like an interpreted language or JIT when you use the "Run Mode", the compiler is actually doing AOT compilation behind the scenes:
*   **Run Mode (`./np main.np`):** Compiles to a temporary binary, executes it instantly, and deletes the binary right after. This gives a Python-like scripting experience with C++ performance.
*   **Build Mode (`./np build main.np`):** Compiles and produces a permanent executable binary (`app.out`) for deployment, similar to Go.

---

## 🛡️ Types Binding (C++ Mapping)

To achieve near-native C++ performance, `np-lang` enforces static typing and maps data types strictly:

*   `int` ➡️ `int64_t`
*   `float` ➡️ `double`
*   `string` ➡️ `std::string`
*   `array` ➡️ `std::vector<np_var>` (Managed via `std::shared_ptr`)
*   `dict` ➡️ `std::map<std::string, np_var>` (Managed via `std::shared_ptr`)

> _Note: There is absolutely NO implicit casting between types (e.g., string + int). This guarantees maximum runtime safety._
