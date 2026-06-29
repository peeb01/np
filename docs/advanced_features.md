# Advanced Features in NP Language

This document describes the implementation, usage, and under-the-hood mechanisms of NP's advanced features.

---

## 1. Pythonic Comprehensions

NP supports elegant list and dictionary comprehensions, compiling them into highly efficient loops that build collection objects dynamically.

### List Comprehensions
Enables mapping and filtering collections in a single line.
*Syntax: `[expression for item in range/array if condition]`*

```python
# 1. Basic Mapping: Multiply numbers by 2
array A = [x * 2 for x in range(1, 5)]
print(A)  # -> [2, 4, 6, 8]

# 2. Filtering: Select values greater than 4
array filtered = [x for x in A if x > 4]
print(filtered)  # -> [6, 8]
```

### Dictionary Comprehensions
Constructs key-value mappings dynamically.
*Syntax: `{key_expression: value_expression for item in range/array}`*

```python
array A = [2, 4, 6, 8]
# Map each integer x to string key and map its value multiplied by 10
dict d = {x: x * 10 for x in A}
print(d)  # -> {"2": 20, "4": 40, "6": 60, "8": 80}
```

---

## 2. Modules & Import System

The `import` keyword allows you to modularize your code. The compiler parses imported files recursively, merging functions, variables, and struct definitions, and includes circular dependency protection.

### Import Path Resolution Order
When importing a file (e.g., `import "math_helper.np"`):
1.  **Local Path Lookup**: The compiler first searches for `"math_helper.np"` relative to the current file's directory.
2.  **Packages Folder Lookup**: If not found locally, the compiler checks the project's dependency cache directory: `.np_packages/math_helper.np`.

### Example
Library file `math_helper.np`:
```python
fn double_val(int v) -> int:
    return v * 2
```

Main file:
```python
import "math_helper.np"

print(double_val(21))  # -> 42
```

---

## 3. Custom Structures (`struct`)

Structs are lightweight data models. In NP, structs are implemented as typed dictionary representations under the hood, enabling field initialization, reading, and mutation using standard dot-notation (`.`).

### Declaring and Instantiating Structs
```python
struct User:
    string name
    int age
    bool is_admin

# Instantiation via automatically generated constructor
User u = User("Bob", 25, true)

# Reading fields using dot-notation
print("Name:", u.name)        # -> Name: Bob
print("Is Admin:", u.is_admin) # -> Is Admin: true

# Modifying fields
u.age = 26
print("New Age:", u.age)      # -> New Age: 26
```

---

## 4. 128-bit & 256-bit Signed Integers

NP supports very large integers, making it suitable for cryptography, checksums, and high-precision scientific calculations.

### A. 128-bit Integers (`int128`)
*   **LLVM Backing**: Maps directly to GCC's native `__int128` signed type.
*   **Operators**: Supports all standard operations (`+`, `-`, `*`, `/`, `%`) and comparison operators natively at compiler speed.
*   **Formatting**: Handled by custom runtime string conversions.

```python
int128 big1 = 123456789012345678901234567890
int128 big2 = 2
int128 res = big1 * big2
print("128-bit Result: ", res)  # -> 128-bit Result: 246913578024691357802469135780
```

### B. 256-bit Integers (`int256`)
*   **LLVM Backing**: Backed by a custom C++ software-implemented 256-bit integer structure in the C++ runtime.
*   **Operations**: Supports addition, subtraction, multiplication, division, modulo, and comparisons.
*   **String formatting**: Pre-compiled inside `runtime/libnpruntime.a`.

```python
int256 v256 = 100000000000000000000000000000000000000000000000000000000000
int256 div_res = v256 / 3
print("256-bit Division Result: ", div_res)
# -> 256-bit Division Result: 33333333333333333333333333333333333333333333333333333333333
```

---

## 5. Exception Handling

NP provides exceptions to recover gracefully from runtime errors (e.g., division by zero, missing files, or explicit throws).

### Syntax
*   `try`: Defines a block of code to monitor for exceptions.
*   `throw`: Explicitly raises a runtime exception carrying a string message.
*   `except Exception e`: Catches any thrown exception, binding the error message to variable `e`.

### Example
```python
fn safe_divide(int a, int b) -> int:
    if b == 0:
        throw "Division by zero error!"
    return a / b

try:
    print("Trying division...")
    int result = safe_divide(10, 0)
    print("Result:", result)
except Exception e:
    print("Caught exception: " + e)
```

*Expected Output:*
```text
Trying division...
Caught exception: Division by zero error!
```
