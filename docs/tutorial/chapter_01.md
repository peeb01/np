# Chapter 1: Introduction and Environment Setup

NP is a high-level scripting language characterized by its clean, Python-like syntax. Behind the scenes, the NP compiler compiles `.np` code into optimized C++17, which is then compiled directly to native machine binaries by GCC. This provides a Python-like scripting experience with the raw performance of native C++, backed by an integrated Garbage Collector.

---

## 1. Preparing Your System

Before building the NP compiler, ensure your system has a C++ compiler and the Boehm GC library installed.

### For Ubuntu / Debian / WSL
Run the following commands in your Terminal:
```bash
sudo apt-get update
sudo apt-get install -y build-essential libgc-dev
```

### For macOS
Install using Homebrew:
```bash
brew install bdw-gc gcc
```

---

## 2. Compiling the NP Transpiler

Once the prerequisites are installed, clone the repository and run `make` inside the project root:
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
*The transpiler converts the source code, compiles it into a temporary binary, executes it, and cleans up the temporary files automatically.*

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
