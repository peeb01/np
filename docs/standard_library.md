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

---

## Standard Modules

Standard modules must be explicitly imported using the `import <module>` statement before they can be used.

### 1. `sys` Module
Provides access to variables and functions that interact with the system environment.

* **`sys.argv` (or `sys.args`) -> array**
  An array containing the command-line arguments passed to the script, starting with the program name as `sys.argv[0]`.

```python
import sys

print("Total arguments:", len(sys.argv))
print("First argument:", sys.argv[1])
```

### 2. `time` Module
Provides utilities for working with timestamps, delays, and date/time formatting.

* **`time.now() -> float`**
  Returns the current Unix timestamp in seconds as a float/double since January 1, 1970 (epoch).
* **`time.sleep(float secs)`**
  Suspends execution of the current thread for the given number of seconds.
* **`time.format(float timestamp, string format_str) -> string`**
  Formats the given Unix timestamp using C-style `strftime` format codes (e.g., `%Y-%m-%d %H:%M:%S`).

```python
import time

float start = time.now()
time.sleep(0.5)
float end = time.now()
print("Elapsed time:", end - start)

string readable = time.format(start, "%Y-%m-%d %H:%M:%S")
print("Started at:", readable)
```

### 3. `json` Module
Provides serialization and parsing functionality for JSON format. Supports integers, floats, strings, booleans, arrays, and nested dictionaries.

* **`json.stringify(var value) -> string`** (alias: **`json.marshal`**)
  Serializes the given value (typically a dictionary or array) into a JSON string.
* **`json.parse(string src) -> var`** (alias: **`json.unmarshal`**)
  Parses a JSON-encoded string and returns its corresponding NP type structure.

```python
import json

dict data = {"id": 101, "tags": ["admin", "verified"]}
string text = json.stringify(data)
print(text)  # -> {"id":101,"tags":["admin","verified"]}

dict parsed = json.parse(text)
print(parsed["tags"][0])  # -> "admin"
```

### 4. `regex` Module
Provides basic regular expression operations.

* **`regex.match(string pattern, string text) -> bool`**
  Returns true if the regular expression pattern matches the entire text; otherwise false.
* **`regex.find(string pattern, string text) -> string`**
  Searces for the first match of the regular expression pattern inside the text and returns it. Returns an empty string if no match is found.
* **`regex.replace(string pattern, string repl, string text) -> string`**
  Replaces all occurrences matching the pattern inside the text with the replacement string.

```python
import regex

bool valid_email = regex.match("^[a-z]+@[a-z]+\\.[a-z]+$", "bob@mail.com")  # -> true
string phone = regex.find("[0-9]{3}-[0-9]{3}-[0-9]{4}", "Call 123-456-7890 tomorrow")  # -> "123-456-7890"
string hidden = regex.replace("[0-9]", "*", "Pin is 1234")  # -> "Pin is ****"
```
