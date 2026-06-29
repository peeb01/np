# Chapter 1: Introduction and Environment Setup

NP is a high-level scripting language characterized by its clean, Python-like syntax. Behind the scenes, the NP compiler parses your source code and uses the **LLVM C++ API** to generate highly optimized machine-code instructions directly, combining it with a static C++ runtime library (`runtime/libnpruntime.a`). This provides a Python-like scripting experience with the raw execution speeds of native C++ and automatic reference counting.

---

## 1. Preparing Your System

Depending on whether you want to **develop the compiler itself** or **just write programs using NP**, the requirements are different:

### A. For Language Users (Writing NP Programs)
If you download a pre-built release binary of the `np` compiler, you **DO NOT** need to install LLVM. You only need a standard C++ linker (like `g++` or `clang` from GCC/Clang) installed in your PATH to link final binaries.
*   **Ubuntu / Debian / WSL**: `sudo apt-get install -y build-essential`
*   **macOS**: Xcode Command Line Tools (`xcode-select --install`)
*   **Windows**: MinGW-w64 (such as GCC/g++)

### B. For Compiler Developers (Building the Compiler from Source)
If you want to modify and compile the C++ source code of the `np` compiler itself, you need the LLVM development libraries:
*   **Ubuntu / Debian / WSL**: `sudo apt-get install -y build-essential llvm-dev`
*   **macOS**: `brew install llvm gcc`

---

## 2. Compiling the NP Compiler (For Developers)

If you are building the compiler from source, clone the repository and run `make` inside the project root:
```bash
make re
```
This generates the native compiler executable named `np` in the workspace root.

---

## 3. Writing Your First Program (Hello World)

Create a new file named `hello.np` and add the following line of code:
```python
print("Hello, World!")
```

### Execution in Scripting Mode (Run)
To run your code immediately without leaving binary files behind:
```bash
./np hello.np
```
Output:
```text
Hello, World!
```
*The compiler translates the source code to LLVM IR, compiles it into a temporary binary, executes it, and cleans up the temporary files automatically.*

### Execution in Compilation Mode (Build)
To compile your source code into a standalone native executable:
```bash
./np build hello.np
```
This creates a standalone binary named `app.out`, which you can execute directly on your host machine:
```bash
./app.out
```

---

[<- Back to Table of Contents](README.md) | [Next: Chapter 2 - Variables and Basic Data Types ->](chapter_02.md)
