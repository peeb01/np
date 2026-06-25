# Chapter 4: Functions and Scoping Rules

Functions allow you to isolate code blocks, parameterize behaviors, and prevent code duplication. They help keep your codebase clean and modular.

---

## 1. Declaring Functions

Use the `fn` keyword to declare functions, specifying parameters, parameter types, and an optional return arrow (`->`) with its type.

### Functions Without Return Value (Void)
```python
fn greet_user(string name):
    print("Hello, " + name)
    print("Welcome to the application!")
```

### Functions With Return Value
```python
fn multiply(int a, int b) -> int:
    return a * b
```

---

## 2. Calling Functions

Once a function is defined, you can call it from anywhere in your program by passing the expected arguments:

```python
# Calling a void function
greet_user("Alice")

# Calling a return function and binding the result
int result = multiply(6, 7)
print("Result:", result)  # -> 42
```

---

## 3. Scoping Rules

Variables declared in NP reside in specific regions of memory depending on where they are defined:

*   **Global Variables:** Declared outside of any function block. They can be read from any scope within the same module.
*   **Local Variables:** Declared inside a function block or loop. They are allocated on the stack when the block is entered and automatically cleaned up when the function returns or the block exits.

```python
int x = 100  # Global Variable

fn test():
    int y = 200  # Local Variable (visible only inside test)
    print("Accessing x:", x)
    print("Accessing y:", y)

test()
# Accessing y here in the global scope would trigger a compilation error.
```

---

[<- Back to Chapter 3](chapter_03.md) | [Next: Chapter 5 - Collections and Slicing ->](chapter_05.md)
