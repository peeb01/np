# NP Language Specification

This document provides a comprehensive guide to the core syntax, grammar, and type system of the NP Language.

---

## Type System

NP supports both **inferred dynamic variables** and **explicitly declared types**. It maps directly to modern C++ types underneath:

| NP Type | C++ Underlying Type | Description |
| :--- | :--- | :--- |
| `int` | `int64_t` | 64-bit signed integer |
| `int32` | `int32_t` | 32-bit signed integer |
| `int64` | `int64_t` | 64-bit signed integer |
| `int128` | `np_int128` | Custom 128-bit signed integer |
| `int256` | `np_int256` | Custom software-implemented 256-bit integer |
| `float` | `double` | 64-bit double-precision float |
| `float32` | `float` | 32-bit single-precision float |
| `float64` | `double` | 64-bit double-precision float |
| `string` | `np_string` | Wrapper inheriting from `std::string` |
| `bool` | `bool` | Boolean (`true` / `false`) |
| `array` | `np_var` | Heap-allocated vector, managed by Boehm GC |
| `dict` | `np_var` | Heap-allocated map, managed by Boehm GC |

---

## Variables & Scope

Dynamic variables are declared with type `var` or mapped to their literal type:
```python
int a = 10
string message = "Hello world"
float pi = 3.14
bool active = true
```

*Note: Variables defined without type default to `np_var` which acts as a dynamic type container.*

---

## Control Flow

NP implements Python-style blocks defined by indentation (4 spaces or tabs) and colons (`:`).

### 1. Conditionals (`if`, `elif`, `else`)
```python
int x = 20
if x > 30:
    print("Greater than 30")
elif x == 20:
    print("Equal to 20")
else:
    print("Less than 20")
```

### 2. While Loops
```python
int count = 0
while count < 5:
    print("Count:", count)
    count = count + 1
```

### 3. For Loops (Range and Iterator)
```python
# Range-based Loop (exclusive upper bound)
for i in range(1, 5):
    print("Range Index:", i)

# Collection Iterator Loop
array fruits = ["apple", "banana", "cherry"]
for fruit in fruits:
    print("Fruit:", fruit)
```

---

## Functions

Functions are defined using the `fn` keyword, with parameter types and optional return arrow (`->`):
```python
fn add_numbers(int x, int y) -> int:
    return x + y

fn greet(string name):
    print("Hello, " + name)
```

---

## Slicing

NP supports vectorized slicing for arrays and strings using the `[start:end]` format, with support for negative indexing:
```python
array numbers = [10, 20, 30, 40, 50]
print(numbers[1:4])   # -> [20, 30, 40]
print(numbers[:3])    # -> [10, 20, 30]
print(numbers[2:])    # -> [30, 40, 50]
print(numbers[-2:])   # -> [40, 50]

string text = "abcdefg"
print(text[2:5])      # -> "cde"
```
