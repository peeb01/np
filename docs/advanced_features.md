# Advanced Features in NP Language

This document describes the implementation and usage of NP's advanced features.

---

## 1. Pythonic Comprehensions

NP supports list and dictionary comprehensions for filtering and mapping collections dynamically.

### List Comprehensions
```python
array A = [x * 2 for x in range(1, 5)]   # -> [2, 4, 6, 8]
array filtered = [x for x in A if x > 4] # -> [6, 8]
```

### Dictionary Comprehensions
```python
dict d = {x: x * 10 for x in A}          # -> {"2": 20, "4": 40, "6": 60, "8": 80}
```

---

## 2. Standard Libraries & Import System

The `import` keyword allows modularizing code across multiple files. The compiler processes imports recursively and merges definitions (variables, functions, structs) with double-import protection.

### Import Custom Libraries
Suppose you have a library file `math_helper.np`:
```python
fn double_val(int v) -> int:
    return v * 2
```

In your main program:
```python
import "math_helper.np"
print(double_val(21))  # -> 42
```

---

## 3. Custom Structures (`struct`)

Structs in NP allow you to create lightweight, named data models. They are backed by dictionary representations under the hood, enabling dot-notation field access and mutation.

### Defining and Using Structs
```python
struct User:
    string name
    int age
    bool is_admin

# Creation
User u = User("Bob", 25, true)

# Reading fields
print("Name:", u.name)
print("Admin:", u.is_admin)

# Mutating fields
u.age = 26
print("New Age:", u.age)
```

---

## 4. 128-bit & 256-bit Signed Integers

NP supports very large integers native to the compiler, useful for cryptography and scientific simulations.

*   `int128`: Mapped to C++ native `__int128_t`. Supports operations like `+`, `-`, `*`, `/`, `%` and logical comparisons.
*   `int256`: Software-implemented 256-bit integer. Supports arithmetic operations, comparisons, long division, and conversions.

```python
int128 big1 = 123456789012345678901234567890
int128 big2 = 2
int128 res = big1 * big2
print("128-bit result:", res)

int256 v256 = 100000000000000000000000000000000000000000000000000000000000
print("256-bit div:", v256 / 3)
```

---

## 5. Exception Handling

NP features exceptions to recover gracefully from runtime errors (e.g. division by zero, missing files, throw statements).

```python
try:
    print("Executing try block...")
    throw "A custom NP language exception occurred!"
except Exception e:
    print("Caught exception:", e)
```
