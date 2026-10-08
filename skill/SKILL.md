---
name: np-lang
description: Comprehensive reference, syntax specification, and coding guidelines for writing, compiling, and debugging programs in the NP programming language (np-compiler). Use this skill whenever writing, reading, fixing, or architecting NP code.
---

# NP Programming Language (np-lang) Expert Guide

This skill equips AI agents to write, read, debug, and architect idiomatic code in the **NP Programming Language** (`np-lang`).

NP is a modern, high-performance compiled language that combines:
- **Python's clean, indented syntax and ergonomics** (dynamic slicing, f-strings, range loops, dict/list literals)
- **Go's robust concurrency and object model** (channels `<-`, goroutines `go`, structs, pointer receivers, interfaces, `defer`, package imports)
- **C/C++'s raw bare-metal execution speed** (compiled via LLVM 18 with `-O3` PassBuilder optimization, native host vectorization, CUDA GPU kernels)

---

## 1. Toolchain & CLI Commands

NP programs have the `.np` file extension. The compiler CLI is `np`.

| Command | Action | Notes |
| :--- | :--- | :--- |
| `np <file.np>` | **Run directly** (like Python) | Compiles to memory/temp and executes immediately. |
| `np <file.np> -O0` | **Run with no optimizations** | Fast compilation, useful for debugging. |
| `np build <file.np>` | **Build standalone native binary** | Generates `app.out` executable (like `go build`). |
| `np build <file.np> -o <binary>` | **Build with custom output name** | e.g. `np build main.np -o bin/server` |
| `np build -O3 <file.np> -o <binary>` | **Build with maximum `-O3` optimization** | Default in modern NP. Emits SIMD AVX2 host code. |
| `np get` / `np install` | **Install dependencies** | Clones packages listed in `np.req` into `.np_packages/`. |

---

## 2. Program Structure & Entry Point

NP supports two styles of writing programs:

### Style A: Scripting Style (Top-level Execution)
Just like Python, code at the file level executes from top to bottom:
```np
print("Hello, World from NP!")
```

### Style B: C/Go Style `main()` Function (Recommended for Applications)
When a `func main()` or `fn main()` is defined, NP automatically treats it as the application entry point:
```np
import time

func main():
    print("Application starting...")
    float t = time.now()
    print("Current timestamp:", t)
```

With command-line arguments and return exit code:
```np
func main(int argc, *string argv) -> int:
    if argc < 2:
        print("Usage: app <name>")
        return 1
    print("Welcome, " + argv[1])
    return 0
```

---

## 3. Type System & Variables

NP is statically typed with powerful type inference (`:=`).

### 3.1 Primitive Types
- `int`: 64-bit signed integer (`int64`)
- `float`: 64-bit IEEE-754 double precision (`float64`)
- `bool`: Boolean (`true` or `false`)
- `string`: UTF-8 native string
- `nil`: Null reference / pointer sentinel
- `void`: Empty return type

### 3.2 Sized and Unsigned Types
```np
uint8 u8 = 250        # 8-bit unsigned integer (alias: byte)
byte b = 100          # alias for uint8
uint16 u16 = 60000    # 16-bit unsigned
uint32 u32 = 4000000000
uint64 u64 = 18000000000000000000
int8 s8 = -120        # 8-bit signed
int16 s16 = -30000    # 16-bit signed
int32 s32 = -2000000  # 32-bit signed
```

### 3.3 Variable Declaration & Type Inference
```np
# Explicit typed declaration:
int count = 10
string greeting = "Hello"
float pi = 3.14159
bool active = true

# Short type inference (:=) (Go-style, Recommended):
x := 42               # Inferred as int
name := "Alice"       # Inferred as string
is_ready := true      # Inferred as bool
score := 98.5         # Inferred as float

# Keyword 'var' (Python / JS style):
var total = 100
var message = "success"

# Multiple declaration / tuple unpacking:
a, b := 10, 20
```

### 3.4 Pointers & References
NP supports pointer semantics primarily through mutating pointer receiver methods (`*Struct`) and pointer parameters:
```np
struct Point:
    int x
    int y

# Mutating receiver method operates directly on the pointer (*Point)
func (p *Point) translate(int dx, int dy):
    p.x += dx
    p.y += dy

pt := Point(10, 20)
pt.translate(5, -5)
print(pt.x)   # 15
```


### 3.5 Collections: Arrays & Dictionaries
```np
# Dynamic Array / List (Python style):
array numbers = [1, 2, 3, 4, 5]
items := ["apple", "banana", "cherry"]

# Sized array preallocation:
array buffer[1024]

# Array methods & properties:
numbers.append(6)
int count = numbers.len()    # or numbers.length
print("First item:", numbers[0])
print("Last item:", numbers[-1])

# Dynamic Dictionary:
dict user = {
    "name": "Bob",
    "role": "admin",
    "age": 30
}

user["status"] = "active"
print("User name:", user["name"])
```

### 3.6 Generics
Generic functions use bracket notation `[T]`:
```np
func identity[T](T val) -> T:
    return val

func pair_first[T, U](T a, U b) -> T:
    return a

x := identity[int](42)
s := identity[string]("hello")
auto_inferred := identity(999)     # Type argument inferred
```

---

## 4. Operators & Expressions

### 4.1 Arithmetic & Bitwise
- Arithmetic: `+`, `-`, `*`, `/`, `%`
- Exponentiation: `**` or `^` (e.g. `2 ** 10 == 1024`, `3 ^ 2 == 9`)
- Bitwise: `&` (AND), `|` (OR), `^` (XOR), `~` (NOT), `<<` (SHL), `>>` (SHR)
- Compound Assignment: `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`
- Increment / Decrement: `c++`, `c--`

### 4.2 Element-wise Array Arithmetic & Vector Broadcasting
NP has built-in NumPy-style element-wise operations on arrays:
```np
array A = [1, 2, 3]
array B = [10, 20, 30]

print(A + B)       # Outputs: [11, 22, 33]
print(A * B)       # Outputs: [10, 40, 90]
print(A + 10)      # Scalar broadcast: [11, 12, 13]
print(A * 5)       # Scalar broadcast: [5, 10, 15]
```

### 4.3 Slicing
Slice syntax `[start:end]` works on both arrays and strings:
```np
items := [10, 20, 30, 40, 50]
print(items[1:4])   # [20, 30, 40]
print(items[:3])    # [10, 20, 30]
print(items[2:])    # [30, 40, 50]

text := "HelloWorld"
print(text[0:5])    # "Hello"
```

### 4.4 String Interpolation (f-strings)
```np
name := "Kiti"
score := 99
msg := f"Player {name} scored {score} points! Double score: {score * 2}"
```

---

## 5. Control Flow

NP uses indentation (4 spaces) for code blocks.

### 5.1 If / Elif / Else
```np
if score >= 90:
    print("Grade A")
elif score >= 80:
    print("Grade B")
else:
    print("Grade C")
```

Ternary conditional:
```np
status := "adult" if age >= 18 else "minor"
```

### 5.2 While Loops
```np
int i = 0
while i < 5:
    print(i)
    i++
```

### 5.3 For Loops
Range loop (`range(start, end)`):
```np
for i in range(0, 10):
    if i == 3:
        continue
    if i == 8:
        break
    print(i)
```

Foreach loop on collections:
```np
fruits := ["apple", "banana", "cherry"]
for f in fruits:
    print("Fruit:", f)
```

### 5.4 Switch / Case / Default
```np
switch status_code:
    case 200:
        print("OK")
    case 404:
        print("Not Found")
    case 500:
        print("Internal Server Error")
    default:
        print("Unknown Status")
```

### 5.5 Enums
```np
enum Role:
    GUEST
    USER
    ADMIN

# With explicit values:
enum HttpStatus: OK = 200, CREATED = 201, NOT_FOUND = 404

role := Role.ADMIN
if role == Role.ADMIN:
    print("Administrator access granted.")
```

### 5.6 Defer (Go style)
Guarantees execution when the surrounding function returns, in reverse (LIFO) order:
```np
func process_file(string path):
    print("Opening file:", path)
    defer print("Closing file:", path)   # Runs on function return!
    
    print("Reading file content...")
```

### 5.7 Assert
```np
assert total > 0
assert user != nil, "User object must not be nil"
```

---

## 6. Functions

Functions can be declared with `func`, `fn`, or `function`:

### 6.1 Basic Function & Multiple Return Values
```np
func add(int a, int b) -> int:
    return a + b

# Multiple returns (Go style):
func divide(int a, int b) -> int, string:
    if b == 0:
        return 0, "error: division by zero"
    return a / b, ""

# Caller unpacking:
result, err := divide(10, 2)
if err != "":
    print("Error:", err)
else:
    print("Result:", result)
```

### 6.2 First-class Functions & Closures
```np
func double(int n) -> int:
    return n * 2

func apply_op(int n, fn(int) -> int op) -> int:
    return op(n)

# Passing function as value:
res := apply_op(21, double)    # 42

# Higher-order function returning a function:
func get_multiplier() -> fn(int) -> int:
    return double

f := get_multiplier()
print(f(15))                   # 30
```

### 6.3 Forward Declarations
NP's compiler runs a two-pass symbol resolution, meaning functions can call each other regardless of declaration order:
```np
func first():
    second()   # Valid even if second() is defined below

func second():
    print("I was declared second!")
```

---

## 7. Structs, Methods & Interfaces (OOP)

NP adopts Go's clean, composition-based Object-Oriented design without classical inheritance overhead.

### 7.1 Structs & Instantiation
```np
struct User:
    string name
    string email
    int age

# Instantiation:
u1 := User("Alice", "alice@example.com", 28)
print(u1.name)
print(u1.age)
```

### 7.2 Value Receiver Methods
Methods that read fields:
```np
struct Rectangle:
    int width
    int height

func (r Rectangle) area() -> int:
    return r.width * r.height

func (r Rectangle) perimeter() -> int:
    return 2 * (r.width + r.height)

rect := Rectangle(10, 20)
print("Area:", rect.area())           # 200
```

### 7.3 Mutating Pointer Receiver Methods (`*Struct`)
Methods that modify the struct in-place:
```np
func (r *Rectangle) scale(int factor):
    r.width *= factor
    r.height *= factor

rect.scale(2)
print("New width:", rect.width)       # 20
```

### 7.4 Interfaces & Polymorphism
Interfaces define method contracts. Any struct implementing the methods satisfies the interface automatically:
```np
interface Shape:
    func area() -> int
    func perimeter() -> int

struct Square:
    int side

func (s Square) area() -> int:
    return s.side * s.side

func (s Square) perimeter() -> int:
    return 4 * s.side

# Polymorphic function accepting ANY Shape:
func describe_shape(string name, Shape s):
    print("Shape:", name)
    print("Area:", s.area())
    print("Perimeter:", s.perimeter())

describe_shape("MySquare", Square(5))
describe_shape("MyRectangle", Rectangle(4, 6))
```

---

## 8. Concurrency & Parallelism

NP offers two powerful concurrency paradigms:
1. **Go-style Channels & Goroutines** (for message passing and pipelines)
2. **Hardware Concurrency Module (`threads`)** (for parallel CPU worker pools)

### 8.1 Channels & Goroutines (`make_chan`, `go`, `<-`)
```np
func worker(void ch):
    time_sleep(0.1)
    ch <- 42               # Send into channel

ch := make_chan(1)         # Buffered channel of capacity 1
go worker(ch)              # Spawn background goroutine

result := <-ch             # Receive from channel (blocks until available)
print("Result from worker:", result)
chan_close(ch)
```

### 8.2 Threads Module (`import threads`)
```np
import threads

func heavy_calc(int x, int y) -> int:
    return x * y + 10

# Check available CPU cores:
num_cores := threads.num_cpu()
print("CPU cores available:", num_cores)

# Spawn asynchronous parallel task on thread pool:
task := threads.run(heavy_calc, 10, 20)
ans := task.wait()         # Wait for completion
print("Answer:", ans)      # 210

# Memory isolation mode (deep-clones arguments so caller state is untouched):
my_list := [1, 2, 3]
task_iso := threads.run(modify_list, my_list, isolated = true)
task_iso.wait()
```

---

## 9. GPU Accelerated Computing (`import gpu`)

NP features built-in NVIDIA CUDA kernel compilation. It generates PTX assembly and loads `libcuda.so.1` directly at runtime with **zero external CUDA Toolkit dependencies**.

```np
import gpu
import time

# GPU Kernel definition:
kernel func vector_add(a, b, c, n: int):
    idx := gpu.thread_idx_x() + gpu.block_idx_x() * gpu.block_dim_x()
    if idx < n:
        c[idx] = a[idx] + b[idx]

func main():
    if !gpu.is_available():
        print("No CUDA GPU detected.")
        return

    print("GPU Device:", gpu.device_name(0))
    int n = 1000000

    a := []
    b := []
    c := []
    for i in range(0, n):
        a.append(1.0)
        b.append(2.0)
        c.append(0.0)

    int block = 256
    int grid = (n + block - 1) / block

    # Launch kernel on GPU:
    gpu.launch(vector_add, grid=grid, block=block, a, b, c, n)

    print("Result C[0]:", c[0])        # 3.0
    print("Result C[n-1]:", c[n - 1])  # 3.0
```

---

## 10. Standard Library Modules

### 10.1 `time`
```np
import time

float now = time.now()       # Current epoch timestamp (seconds)
time.sleep(0.5)              # Sleep for 500 ms
```

### 10.2 `json`
```np
import json

data := {"user": "Alice", "score": 100, "verified": true}

# Serialize:
json_string := json.stringify(data)
print("JSON:", json_string)

# Deserialize:
parsed := json.parse(json_string)
print("User:", parsed["user"])
```

### 10.3 `os`
```np
import os

# Execute command and capture stdout:
output := os.exec("git status -s")
print("Git status:\n" + output)

# Run shell command and get exit code:
exit_code := os.system("ls -l")

# Environment variables:
home := os.getenv("HOME")
```

### 10.4 `regex`
```np
import regex

is_match := regex.match("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$", "test@domain.com")
ip := regex.find("[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+", "Connect to 192.168.1.1:8080")
cleaned := regex.replace("[0-9]+", "X", "User12345")
```

### 10.5 `sys`
```np
import sys

argc := len(sys.argv)
exe_name := sys.argv[0]
```

### 10.6 `crypto`
```np
import crypto

hash := crypto.sha256("password123")
print("SHA256:", hash)
```

### 10.7 File I/O (Built-in)
```np
# Write file (returns 1 on success, 0 on failure):
ok := write_file("config.txt", "server=localhost\nport=8080\n")

# Read file:
content := read_file("config.txt")
print("Content:\n" + content)
```

---

## 11. Package Management (`np get`)

NP supports remote package installation directly from Git repositories:
1. Create a `np.req` file in the project root:
   ```
   github.com/peeb01/np-math
   github.com/peeb01/np-web
   ```
2. Run `np get` or `np install`:
   ```bash
   np get
   ```
3. Import packages in code:
   ```np
   import "github.com/peeb01/np-math"
   import "github.com/peeb01/np-math" as mathlib
   ```

---

## 12. Best Practices & Idiomatic Guidelines for AI

When writing NP code:

1. **Prefer `:=` for local variable declaration**:
   - Write `total := 0` instead of `int total = 0` unless explicit typing is needed.
2. **Always use 4-space indentation**:
   - NP's lexer parses Python-style indentation blocks. Never mix tabs and spaces.
3. **Use Go-style multiple returns for error handling**:
   - `func find(string key) -> string, bool:`
   - `val, ok := find("token")`
4. **Take advantage of `-O3` Recursion & Loop Optimization**:
   - NP's LLVM PassBuilder pipeline optimizes linear recursions and accumulator patterns into CPU register loops ($O(1)$ stack).
5. **Use Mutating Pointer Receivers `(*Struct)` when modifying state**:
   - `func (u *User) set_age(int age): u.age = age`
6. **Use `defer` for cleanup operations**:
   - Ensures memory or file handles are closed even if early returns occur.
7. **Use F-strings for readable string formatting**:
   - Write `f"ID: {id}, Name: {name}"` instead of long `+` chains.

---

## 13. Canonical Verified Reference Examples

Refer to these tested and verified templates in the `skill/examples/` folder:
- **`skill/examples/01_basics.np`**: Variables, type inference (`:=`), arrays, f-strings, control flow, functions, multiple return values.
- **`skill/examples/02_oop_interfaces.np`**: Structs, value receivers, mutating pointer receivers (`*Struct`), interfaces, polymorphism.
- **`skill/examples/03_concurrency.np`**: Go-style channels (`make_chan`, `<-`), background goroutines (`go`), thread pool (`threads.run`, `task.wait()`).
- **`skill/examples/04_stdlib.np`**: Built-in modules (`sys`, `time`, `json`, `regex`).

