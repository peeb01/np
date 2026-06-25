# NP Compiler & Language Reference

Welcome to the **NP Compiler** project! NP is a lightweight scripting language designed to combine Python-style clean syntax (indentation-based blocks, comprehensions, and structures) with native C++ execution speeds and automated Garbage Collection.

---

## Language Documentation

To understand the language features in detail, check out the following sub-documents:
*   **[Getting Started Guide](docs/getting_started.md):** Installation, host setup, run/build commands, and Docker execution.
*   **[Language Specification](docs/language_specification.md):** Core syntax, variables, scopes, type rules, conditionals, loops, functions, and slicing.
*   **[Advanced Features Guide](docs/advanced_features.md):** Structs, import modules, Pythonic list/dict comprehensions, 128/256-bit signed integers, and try/except exceptions.
*   **[Standard Library Reference](docs/standard_library.md):** Built-in utility functions, File I/O helpers, and methods for strings and arrays.
*   **[NP Programming Tutorial Book](docs/tutorial/README.md):** A step-by-step programming book covering NP basics to advanced topics.

---

## Translation Pipeline (How it works)

NP acts as an **Ahead-Of-Time (AOT) Transpiler (Source-to-Source Compiler)**. It compiles `.np` code into optimized standard C++17, which is then compiled into a native machine-code binary by GCC (`g++`).

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

## Quick Start with Docker

You can compile and run NP code instantly via Docker. Mount your current directory (`$PWD`) to the container's `/workspace`:

### Linux / macOS (Bash/Zsh):
```bash
alias np='docker run --rm -it -v "$PWD":/workspace pib21/np-lang:alpine-3.22'

# Run immediately (interpreting style)
np tests/advanced_features_test.np

# Compile to a binary (AOT)
np build tests/advanced_features_test.np
```

---

## Core Mechanisms & Features

### 1. Single-Pass Recursive Descent Parser
NP uses a hand-written parser that translates syntax rules directly into C++ strings on the fly. This results in incredibly fast compilation times.

### 2. Automatic Memory Management (Boehm GC)
*   **Stack Allocation:** Primitive types (`int`, `float`, `bool`, `string`) are mapped to C++ primitives and stack-allocated, leaving no footprint.
*   **Garbage Collection:** Complex data structures (`array`, `dict`) are heap-allocated raw pointers managed seamlessly by the **Boehm Garbage Collector** (`-lgc`). No manual memory management or ARC references needed.

### 3. Built-in 128-bit & 256-bit Integers
*   `int128` maps directly to GCC's native `__int128` type.
*   `int256` is a software-implemented 256-bit signed integer supporting math, comparisons, division, and modulo natively in the compiler.
