# NP Standard Library Reference

This document provides a comprehensive guide to all built-in functions, user input utilities, collection methods, and standard library modules (`sys`, `time`, `json`, `regex`) available in the NP Language.

---

## 1. File I/O Functions

### A. `read_file(string filename) -> string`
Reads the entire contents of a file and returns it as a string. If the file does not exist or cannot be opened, it returns an empty string `""`.
*   **Parameters:** `filename` - The absolute or relative path to the file.
*   **Returns:** A `string` containing the file content.

```python
string text = read_file("test.txt")
print("File Contents: " + text)
```

### B. `write_file(string filename, string content) -> int`
Writes a string into a file, overwriting its existing contents. Creates the file if it does not exist.
*   **Parameters:**
    *   `filename` - The target path.
    *   `content` - The string content to write.
*   **Returns:** `1` if the write succeeded, `0` if it failed.

```python
int success = write_file("output.txt", "Hello World from NP!")
if success == 1:
    print("Write succeeded!")
```

---

## 2. Utility & Type Functions

### A. `print(var1, var2, ...)`
Prints any number of arguments to the standard output, separated by newlines or formatting.
*   **Parameters:** Variadic list of variables or literals (`int`, `float`, `bool`, `string`, `array`, `dict`).
*   **Returns:** `void`

```python
print("PI:", 3.14159, "Active:", true)
```

### B. `len(var collection) -> int`
Returns the length of a string, or the number of elements in an array or dictionary.
*   **Parameters:** A `string`, `array`, or `dict`.
*   **Returns:** `int` containing the size.

```python
print(len("HelloWorld"))         # -> 10
print(len([10, 20, 30]))         # -> 3
print(len({"name": "Bob", "age": 25})) # -> 2
```

### C. `type(var variable) -> string`
Inspects a variable at runtime and returns a string representing its current type.
*   **Parameters:** Any variable.
*   **Returns:** One of `"int"`, `"float"`, `"string"`, `"bool"`, `"array"`, `"dict"`, `"int128"`, `"int256"`.

```python
print(type(42))            # -> "int"
print(type("Hello"))       # -> "string"
print(type([1, 2, 3]))     # -> "array"
```

---

## 3. User Input Functions

These functions read lines from the standard input (`std::cin`), parsing them into primitives:

### A. `input_string() -> string`
Reads a full line of text from standard input.
*   **Returns:** A `string` containing the input line.

```python
print("Enter your name:")
string name = input_string()
print("Hello, " + name)
```

### B. `input_int() -> int`
Reads standard input and parses it into a 64-bit integer.
*   **Returns:** An `int` value.

```python
print("Enter age:")
int age = input_int()
print("Next year you will be: " + string(age + 1))
```

### C. `input_float() -> float`
Reads standard input and parses it into a double-precision float.
*   **Returns:** A `float` value.

```python
print("Enter exchange rate:")
float rate = input_float()
print("Rate is: " + string(rate))
```

---

## 4. Array Methods

These methods are called directly on variables of type `array`:

### A. `arr.append(var value)`
Appends a dynamic or primitive value to the end of the array. Modifies the array in-place.
*   **Parameters:** `value` - Any variable or literal to append.

```python
array nums = [1, 2]
nums.append(3)
print(nums)  # -> [1, 2, 3]
```

### B. `arr.pop() -> var`
Removes and returns the last element of the array. Throws an exception if the array is empty.
*   **Returns:** The popped `var` element.

```python
array nums = [10, 20, 30]
var last = nums.pop()
print("Popped:", last) # -> Popped: 30
print(nums)            # -> [10, 20]
```

### C. `arr.sort()`
Sorts the elements of the array in-place in ascending order.
```python
array nums = [30, 10, 20]
nums.sort()
print(nums)  # -> [10, 20, 30]
```

### D. `arr.reverse()`
Reverses the order of the elements in the array in-place.
```python
array nums = [1, 2, 3]
nums.reverse()
print(nums)  # -> [3, 2, 1]
```

### E. `arr.contains(var value) -> bool`
Checks if the specified value is present in the array.
*   **Returns:** `true` if found, `false` otherwise.

```python
array names = ["Alice", "Bob"]
print(names.contains("Alice"))  # -> true
print(names.contains("Charlie")) # -> false
```

---

## 5. String Methods

These methods are called directly on variables of type `string`:

### A. `s.trim() -> string`
Trims leading and trailing whitespace characters (spaces, tabs, newlines).
*   **Returns:** A new trimmed `string`.

```python
string spaced = "   hello   "
print("'" + spaced.trim() + "'")  # -> 'hello'
```

### B. `s.split(string delimiter) -> array`
Splits a string around occurrences of the given delimiter.
*   **Parameters:** `delimiter` - The splitter token.
*   **Returns:** An `array` containing the split substrings.

```python
string csv = "apple,banana,orange"
array fruits = csv.split(",")
print(fruits)  # -> ["apple", "banana", "orange"]
```

### C. `s.join(array parts) -> string`
Joins elements of an array of strings, using the caller string as the glue separator.
*   **Parameters:** `parts` - An array of strings.
*   **Returns:** The joined `string`.

```python
string glue = "-"
string joined = glue.join(["a", "b", "c"])
print(joined)  # -> "a-b-c"
```

### D. `s.contains(string substring) -> bool`
Checks if a substring is present within the string.
*   **Returns:** `true` if found, `false` otherwise.

```python
string sentence = "Quick brown fox"
print(sentence.contains("brown")) # -> true
```

---

## 6. Standard Modules

Standard modules are loaded using the `import <module>` statement.

### A. `sys` Module
Provides access to arguments passed to the program.
*   **`sys.argv` -> array**: An array of command-line arguments. `sys.argv[0]` contains the compiled binary's file path.

```python
import sys

print("Number of args:", len(sys.argv))
for arg in sys.argv:
    print(arg)
```

### B. `time` Module
Utility functions to measure time, sleeps, and date formatting.

*   **`time.now() -> float`**: Returns the current Unix timestamp in seconds.
*   **`time.sleep(float seconds)`**: Suspends the program thread for the given duration.
*   **`time.format(float timestamp, string format_str) -> string`**: Formats a timestamp using C-style `strftime` codes.

```python
import time

float start = time.now()
time.sleep(0.5)
float elapsed = time.now() - start
print("Elapsed seconds:", elapsed)

print(time.format(time.now(), "%Y-%m-%d %H:%M:%S")) # -> 2026-06-29 09:30:00
```

### C. `json` Module
JSON serialization and parsing.

*   **`json.stringify(var value) -> string`**: Serializes an array or dictionary into a compact JSON string.
*   **`json.parse(string source) -> var`**: Parses a JSON string, rebuilding the nested array/dictionary structures.

```python
import json

dict data = {"name": "Alice", "tags": ["admin", "verified"]}
string raw = json.stringify(data)
print(raw)  # -> {"name":"Alice","tags":["admin","verified"]}

dict parsed = json.parse(raw)
print(parsed["tags"][0])  # -> "admin"
```

### D. `regex` Module
Provides POSIX-compliant regular expression matching, searching, and replacing.

*   **`regex.match(string pattern, string text) -> bool`**: Returns true if the regex pattern matches the entire text.
*   **`regex.find(string pattern, string text) -> string`**: Returns the first matching substring within the text.
*   **`regex.replace(string pattern, string replacement, string text) -> string`**: Replaces all matching substrings with the replacement text.

```python
import regex

# Verify email address format
print(regex.match("^[a-z]+@[a-z]+\\.[a-z]+$", "test@mail.com"))  # -> true

# Find telephone number pattern
print(regex.find("[0-9]{3}-[0-9]{3}", "Call 123-456 now"))      # -> "123-456"

# Hide numbers
print(regex.replace("[0-9]", "*", "Password is 1234"))          # -> "Password is ****"
```

---

## 7. Low-Level Network Socket Functions (net)

NP provides native TCP networking primitives. To keep C++ involvement minimal and optimize for raw execution speeds, sockets are represented as raw integer file descriptors (`fd`) mapping directly to POSIX/WinSock sockets.

### A. `net_listen(int port) -> int`
Opens a TCP socket, binds to all interfaces (`0.0.0.0`) on the specified port, and listens for incoming connections.
*   **Parameters:** `port` - The port to listen on.
*   **Returns:** A raw socket file descriptor (`fd`) as an `int`, or `-1` on error.

```python
int server_fd = net_listen(8080)
```

### B. `net_accept(int server_fd) -> int`
Blocks and accepts an incoming connection from a client.
*   **Parameters:** `server_fd` - The listening socket file descriptor.
*   **Returns:** A new client socket file descriptor (`fd`) as an `int`, or `-1` on error.

```python
int client_fd = net_accept(server_fd)
```

### C. `net_connect(string host, int port) -> int`
Connects to a remote TCP server.
*   **Parameters:**
    *   `host` - The IP address or hostname.
    *   `port` - The target port.
*   **Returns:** A socket file descriptor (`fd`) as an `int`, or `-1` on error.

```python
int client_fd = net_connect("127.0.0.1", 8080)
```

### D. `net_send(int socket_fd, string data) -> int`
Sends string data over the socket.
*   **Parameters:**
    *   `socket_fd` - The target socket descriptor.
    *   `data` - The string payload.
*   **Returns:** The number of bytes successfully sent, or `-1` on error.

```python
net_send(client_fd, "Hello Server")
```

### E. `net_recv(int socket_fd, int max_bytes) -> string`
Receives data from the socket.
*   **Parameters:**
    *   `socket_fd` - The target socket descriptor.
    *   `max_bytes` - The maximum bytes to read.
*   **Returns:** A `string` containing the received data, or an empty string `""` on EOF or error.

```python
string msg = net_recv(client_fd, 1024)
```

### F. `net_close(int socket_fd)`
Closes the socket file descriptor and releases system resources.
*   **Parameters:** `socket_fd` - The socket descriptor to close.

```python
net_close(client_fd)
```

---

## 8. Network Examples

### TCP Echo Server Example
```python
int server_fd = net_listen(8080)
if server_fd < 0:
    throw "Failed to listen on port 8080"

print("Server is listening on port 8080...")

while true:
    int client_fd = net_accept(server_fd)
    if client_fd >= 0:
        string request = net_recv(client_fd, 1024)
        print("Received: " + request)
        
        string response = "Echo: " + request
        net_send(client_fd, response)
        net_close(client_fd)
```

### TCP Client Example
```python
int client_fd = net_connect("127.0.0.1", 8080)
if client_fd < 0:
    throw "Failed to connect to server"

net_send(client_fd, "hello")
string response = net_recv(client_fd, 1024)
print("Response: " + response)
net_close(client_fd)
```

