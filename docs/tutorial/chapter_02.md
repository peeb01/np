# Chapter 2: Variables, Data Types, and Operations

In NP, variables act as containers for storing values. Variables can be declared dynamically or mapped to explicit C++ static types for extra performance and safety.

---

## 1. Declaring Variables

Variables in NP can be defined in two ways: with explicit type annotations or dynamically.

### Explicitly Typed Declarations
When you know the exact type of data a variable will hold, prefix it with the type keyword:
```python
int count = 10
float price = 99.50
string username = "Alice"
bool is_active = true
```

### Dynamically Typed Declarations
When you declare a variable without specifying its type, it defaults to a dynamic variable container (`np_var`). This container can change types or act as a reference to heap collections:
```python
x = 100
name = "Bob"
```

---

## 2. Core Data Types

1.  **int:** A 64-bit signed integer (e.g. `1`, `-15`, `1000`).
2.  **float:** A double-precision floating-point number (e.g. `3.14`, `-0.005`).
3.  **string:** An array of characters enclosed in quotation marks (e.g. `"hello"`).
4.  **bool:** A boolean logic value containing either `true` or `false`.

---

## 3. Arithmetic Operators and Type Safety

You can perform arithmetic operations using standard symbols:
*   `+` (addition)
*   `-` (subtraction)
*   `*` (multiplication)
*   `/` (division)
*   `%` (modulo)
*   `^` (power/exponent)

### Example:
```python
int a = 10
int b = 3
print("Sum:", a + b)
print("Div:", a / b)
print("Exp:", a ^ b)
```

### Strict Type Safety
To avoid runtime errors, NP enforces strict static checks and does not perform implicit type casting. For example, adding a string to an integer directly is forbidden:
```python
# This statement triggers a TypeError at compile time:
string s = "Result: " + 42
```
To run this correctly, you must explicitly convert the integer using cast utilities (which we will cover in later chapters).

---

[<- Back to Chapter 1](chapter_01.md) | [Next: Chapter 3 - Control Flow ->](chapter_03.md)
