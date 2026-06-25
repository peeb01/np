# NP Standard Library Reference

NP provides built-in functions and methods to work with standard data structures (strings, arrays, files).

---

## File I/O Functions

### 1. `read_file(string filename) -> string`
Reads the entire content of a file and returns it as a string. If the file cannot be opened, it returns an empty string.
```python
string text = read_file("config.txt")
```

### 2. `write_file(string filename, string content) -> int`
Writes the specified content to a file. Returns `1` on success and `0` on failure.
```python
int success = write_file("output.txt", "hello world")
```

---

## Utility Functions

### 1. `print(var1, var2, ...)`
Prints any number of arguments to the standard output, separated by spaces, and ends with a newline.
```python
print("Result is:", 42, true)
```

### 2. `len(collection) -> int`
Returns the number of elements in an array or dictionary, or the length of a string.
```python
int length = len("hello")  # -> 5
```

### 3. `type(variable) -> string`
Returns a string representing the type name of the variable (`int`, `float`, `string`, `bool`, `array`, `dict`, `int128`, `int256`, or `unknown`).
```python
print(type([1, 2, 3]))  # -> "array"
```

---

## Array Methods

These methods are available on variables of type `array`.

### 1. `arr.sort()`
Sorts the array in place in ascending order.
```python
array numbers = [50, 10, 40]
numbers.sort()
print(numbers)  # -> [10, 40, 50]
```

### 2. `arr.reverse()`
Reverses the array elements in place.
```python
numbers.reverse()
print(numbers)  # -> [50, 40, 10]
```

### 3. `arr.contains(value) -> bool`
Returns `true` if the array contains the specified value; otherwise `false`.
```python
bool has_thirty = numbers.contains(30)
```

---

## String Methods

These methods are available on variables of type `string`.

### 1. `s.trim() -> string`
Removes leading and trailing whitespace characters (spaces, tabs, newlines) and returns the new string.
```python
string text = "   hello   ".trim()  # -> "hello"
```

### 2. `s.split(string delim) -> array`
Splits the string by a delimiter and returns an array of substrings.
```python
array words = "hello,world".split(",")  # -> ["hello", "world"]
```

### 3. `s.join(array parts) -> string`
Joins elements of an array using the string as the separator.
```python
string separator = "-"
string joined = separator.join(["a", "b", "c"])  # -> "a-b-c"
```
