# Chapter 7: File Operations and Exception Handling

Robust programs must interact with external data sources (like files) and recover gracefully from runtime errors rather than crashing abruptly.

---

## 1. File Input/Output Operations

NP provides global helper functions to read and write files directly.

### Writing to a File
Use `write_file(filename, content)` to save text to a file. It returns `1` on success and `0` on error:
```python
int success = write_file("tests/tmp_io.txt", "Hello from NP File I/O!")
if success == 1:
    print("File written successfully!")
```

### Reading from a File
Use `read_file(filename)` to retrieve a file's entire content as a string:
```python
string content = read_file("tests/tmp_io.txt")
print("File Content:", content)
```

---

## 2. Exception Handling

When a fatal runtime error occurs, the program throws an exception. You can intercept these exceptions and keep your program running using a `try`/`except` block:

```python
try:
    print("Initiating operation...")
    # Raising a custom exception using throw
    throw "Database connection timed out!"
    print("This line will never execute.")
except Exception e:
    print("Intercepted error:", e)

print("Program execution continues normally.")
```

When an exception is thrown inside a `try` block, execution immediately jumps to the matching `except` block. The exception message is bound to the target exception variable (`e`), allowing you to log or mitigate the issue.

---

[<- Back to Chapter 6](chapter_06.md) | [Next: Chapter 8 - Advanced Numerics and Project Structure ->](chapter_08.md)
