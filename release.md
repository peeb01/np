# NP Language Compiler Version 1.0.0 Release Notes

We are pleased to announce the official release of the NP programming language compiler (Version 1.0.0). The NP language is a statically typed, compiled programming language utilizing an LLVM backend for high-performance machine code generation. It is designed to combine the simplicity and readability of pythonic syntax with the type safety and execution speed of compiled languages.

This release represents a fully functional, production-ready version of the compiler, runtime library, dependency management tool, and integrated developer environments.

## Compiler and Language Specification

The core compiler compiles NP source code directly to native object files using LLVM, which are then linked using the system compiler (g++) alongside the custom NP static runtime library (libnpruntime.a).

### Key Features
* Statically Typed Variable Declarations: Supports basic types including int, float, string, and bool with fixed bit widths (int32, int64, int128, int256, float32, float64).
* Data Structures: Out-of-the-box support for dynamically sized arrays, key-value dictionaries, and custom data structures (structs).
* Control Flow: Includes python-like control structures such as if-elif-else statements, while loops, and for loops (supporting both range-based iterations and foreach array traversal).
* Functions: Supports full function declarations with single return values, void returns, and multiple return values (destructured variable assignment).
* Explicit Type Casting: Built-in functions to cast between primitives, such as int(), float(), and string().
* Built-in Standard Library APIs:
  * File I/O: High-level read_file() and write_file() utilities.
  * Socket Networking: Native TCP client-server functions (net_listen, net_accept, net_connect, net_send, net_recv, net_close) for building network-based services.
  * Vectorized Slicing: Native syntax for array and string slicing (e.g. array[1:4]).

## On-Demand Remote Package Manager

The package manager resolves dependencies dynamically at compile time.
* Auto-Download Remote Imports: Resolves imports pointing to remote repositories (e.g. import "github.com/user/repo") by downloading packages as ZIP files over HTTP and extracting them locally without external git dependencies.
* Caching: Downloaded packages are cached in the .np_packages directory.
* Directory Entry Point Resolution: Automatically searches for standard entry points (mod.np, main.np, index.np, or <dirname>.np) in directories.
* Namespace Isolation: Inherits and propagates package namespaces to prevent name collisions across third-party libraries.

## Integrated Developer Tooling (VS Code Extension)

The official vscode-np extension provides a rich environment for writing NP code.
* Syntax Highlighting: Full TextMate grammar implementation covering all core keywords, built-ins, types, constants, strings, numbers, and comments.
* Go to Definition: Full workspace-wide definition provider support (Ctrl+Click or F12) that scans local files, workspace files, and downloaded packages in .np_packages.
* File Icon Customization: Custom file icons mapped to .np files (NP logo) and .np.expected test files (NP expected logo) for better project visualization.
* Portability Scripts: Includes setup.bat (Windows) and setup.sh (Linux/macOS/WSL) for one-click installation of the extension on developer environments.

## Test Suite and Verification

The repository now includes a dedicated test runner (tests/run_tests.py) to guarantee compiler correctness.
* Automatically verifies 10 core language feature test cases covering math, stdlib, type conversion, slicing, and import packages.
* Compares actual compiler output against normalized snapshot output (.np.expected files).
* Normalizes non-deterministic memory addresses (pointer values) to prevent flaky test failures under ASLR.
* Integrated directly into the Makefile, forcing test suite validation on every global compilation (make/sudo make).

## System Requirements

### For End-Users (Using Pre-compiled Binary Releases)
To run the pre-compiled `np` compiler to compile `.np` source files into executable binaries, the system only requires:
* g++ (C++ compiler and linker) for the final linking stage.
* LLVM 18 runtime libraries (since the compiler binary links dynamically).

### For Developers (Compiling From Source)
To compile the compiler and runtime library from source:
* LLVM 18 development files (`llvm-dev` / `mingw-w64-ucrt-x86_64-llvm`).
* build-essential toolchain (`gcc`, `g++`, `make`, `ar`).
* Python 3 (only required for running the integrated test suite).

## Installation (Building From Source)
To compile and install the compiler and runtime library globally:
```bash
make
sudo make install
```
This installs the `np` binary to `/usr/local/bin/np` and the static runtime library to `/usr/local/lib/libnpruntime.a`.
