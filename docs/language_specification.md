# NP Language Specification

This document provides a comprehensive specification of the NP Language core syntax, type system, variables, scoping rules, control flow, functions, slicing mechanisms, and destructuring assignments.

---

## 1. Type System

NP supports both **statically typed variables** (e.g., `int`, `float`, `string`, `bool`, `array`, `dict`) and **implicitly typed dynamic variables** (`var` or no type prefix).

### Primitive Types (Stack-Allocated)

| NP Type | LLVM IR Type | Description | Default Value |
| :--- | :--- | :--- | :--- |
| `int` | `i64` | 64-bit signed integer | `0` |
| `int32` | `i64` | Alias for 64-bit signed integer (mapped for compatibility) | `0` |
| `int64` | `i64` | Alias for 64-bit signed integer | `0` |
| `float` | `double` | 64-bit double-precision floating-point number | `0.0` |
| `float32` | `double` | Alias for 64-bit double-precision float | `0.0` |
| `float64` | `double` | Alias for 64-bit double-precision float | `0.0` |
| `bool` | `i1` | 1-bit boolean flag (`true` or `false`) | `false` |

### Complex/Dynamic Types (Heap-Allocated & Reference Counted)

These types compile to generic pointers (`i8*` or `ptr`) in the LLVM IR and are mapped to `np_var` or `np_string` C++ structures in the runtime:

| NP Type | Under the Hood | Description | Default Value |
| :--- | :--- | :--- | :--- |
| `string` | `np_string*` | Dynamic UTF-8 string wrapper around `std::string` | `""` |
| `array` | `np_var*` | Heap-allocated dynamic array (`std::vector<np_var>`) | `[]` |
| `dict` | `np_var*` | Heap-allocated dynamic dictionary (`std::map<std::string, np_var>`) | `{}` |
| `var` | `np_var*` | Container holding any primitive/complex type dynamically | `null` |

---

## 2. Variables & Scoping Rules

### Variable Declarations

#### A. Statically Typed Variables
Declared with an explicit type name. The compiler allocates stack memory via LLVM `alloca` and enforces type correctness at compile time.
```python
int age = 23
float height = 175.5
string name = "Developer"
bool active = true
```

#### B. Dynamic Variables
Declared using `var` or without any type prefix. These compile to dynamic pointers (`np_var*`) initialized at runtime:
```python
var dynamic_x = 100
dynamic_x = "Now I am a string!"
dynamic_x = [1, 2, 3]
```

### Scoping & Lifetimes
1.  **Global Scope**: Variables defined outside of any function are global.
2.  **Local Scope**: Variables declared inside a function or loop body are local to that block. Local variables shadow global variables of the same name.
3.  **Automatic Reclamation (RAII)**: Local primitive variables are discarded when their stack frame pops. Local complex variables (`string`, `array`, `dict`) decrement their runtime reference count on scope exit, releasing heap memory immediately if their count hits zero.

---

## 3. Operators

### Arithmetic Operators
Support standard mathematical operations between similar numeric types:
*   `+` Addition
*   `-` Subtraction / Unary Negation
*   `*` Multiplication
*   `/` Division (returns `int` if both operands are integers, `float` otherwise)
*   `%` Modulo (remainder of integer division)
*   `^` Power operator (e.g., `2 ^ 3` yields `8`)

```python
int a = 10
int b = 3
print(a + b)  # -> 13
print(a - b)  # -> 7
print(a * b)  # -> 30
print(a / b)  # -> 3 (integer division)
print(a % b)  # -> 1
print(2 ^ 3)  # -> 8
```

### Comparison Operators
Evaluate expressions and return a `bool` value:
*   `==` Equal to
*   `!=` Not equal to
*   `<` Less than
*   `>` Greater than
*   `<=` Less than or equal to
*   `>=` Greater than or equal to

```python
print(10 > 5)   # -> true
print(10 == 5)  # -> false
print(10 != 5)  # -> true
```

### Logical Operators
Short-circuiting boolean operations:
*   `and` Returns true if both conditions are true.
*   `or` Returns true if at least one condition is true.
*   `not` Unary negation of boolean value.

```python
bool active = true
bool verified = false
print(active and not verified)  # -> true
print(active or verified)       # -> true
```

---

## 4. Control Flow

NP uses indentation-based block structures (using 4 spaces or tabs) terminated by a colon (`:`).

### 1. Conditionals (`if`, `elif`, `else`)
Executes code blocks based on condition evaluation.
```python
int score = 85

if score >= 90:
    print("Grade A")
elif score >= 80:
    print("Grade B")
else:
    print("Grade F")
```
*Expected Output:*
```text
Grade B
```

### 2. While Loops
Repeatedly executes a block of code as long as a condition is true.
```python
int count = 1
while count <= 3:
    print("Loop iteration:", count)
    count = count + 1
```
*Expected Output:*
```text
Loop iteration: 1
Loop iteration: 2
Loop iteration: 3
```

### 3. For Loops
Supports range-based loops and iterating over array collections.

#### A. Range-based Loop (Exclusive upper bound)
Syntax: `for var in range(start, end):`
```python
for i in range(0, 3):
    print("Index:", i)
```
*Expected Output:*
```text
Index: 0
Index: 1
Index: 2
```

#### B. Collection Iterator Loop
Syntax: `for item in array:`
```python
array items = ["A", "B", "C"]
for x in items:
    print("Item:", x)
```
*Expected Output:*
```text
Item: A
Item: B
Item: C
```

---

## 5. Functions

Functions are defined using the `fn` keyword, specifying parameters with their types, and an optional return type arrow `->`:

### Syntax
```python
fn function_name(type arg1, type arg2) -> return_type:
    # Function body
    return value
```

### Examples

#### Function returning a value
```python
fn add(int x, int y) -> int:
    return x + y

print(add(5, 7))  # -> 12
```

#### Void function (No return value)
```python
fn greet(string name):
    print("Hello, " + name)

greet("NP user")  # -> Hello, NP user
```

---

## 6. Slicing

NP supports Python-style slicing on `string` and `array` types using `[start:end]` format, with support for negative indexes.

### Syntax
*   `x[start:end]`: Slices from index `start` up to (but excluding) `end`.
*   `x[:end]`: Slices from the beginning up to `end`.
*   `x[start:]`: Slices from `start` to the end of the collection.
*   `x[-n:]`: Slices from the $n$-th element from the end to the very end.

### Array Slicing Example
```python
array numbers = [10, 20, 30, 40, 50]
print(numbers[1:4])   # -> [20, 30, 40]
print(numbers[:3])    # -> [10, 20, 30]
print(numbers[2:])    # -> [30, 40, 50]
print(numbers[-2:])   # -> [40, 50]
```

### String Slicing Example
```python
string text = "HelloWorld"
print(text[0:5])      # -> "Hello"
print(text[5:])       # -> "World"
print(text[-5:])      # -> "World"
```

---

## 7. Destructuring Assignments

NP supports assigning multiple return values from a function or unpacking array collections directly into variables.

### Unpacking Function Multiple Returns
Useful for functions returning multiple values (like quotients and remainders).
```python
fn divide(int dividend, int divisor) -> array:
    int quotient = dividend / divisor
    int remainder = dividend % divisor
    return [quotient, remainder]

# Destructuring assignment
int q, int r = divide(10, 3)
print("Quotient:", q)    # -> 3
print("Remainder:", r)   # -> 1
```

### Collection Unpacking
Unpack elements of an array directly into separate local variables:
```python
array coords = [100, 200]
int x, int y = coords
print("X:", x)   # -> 100
print("Y:", y)   # -> 200
```
