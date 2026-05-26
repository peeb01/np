# 🚀 np-lang Syntax Specification (Version 1.0)

Welcome to the official blueprint of **np-lang**, a programming language designed to be as clean and readable as Python, while maintaining the type-safety, speed, and strictness of Go (Explicit Statically-Typed).

---

## 1. Data Types & Variables
The np-lang language enforces explicit type declarations at the beginning of variable definitions. Semicolons `;` are not required at the end of statements.

### 🔹 Supported Primitive Types
* `int` : Integer numbers (e.g., `10`, `-5`)
* `float` : Floating-point numbers (e.g., `3.14`, `-0.5`)
* `string` : Text characters encapsulated in double quotes (e.g., `"Hello np-lang"`)
* `bool` : Boolean values (`true` or `false`)

### 🔹 Examples
```python
int age = 23
float height = 175.5
string name = "Developer"
bool is_running = true
```

2. Basic Operators
🔹 Arithmetic Operators
Supports standard mathematical evaluations.

```Python
int a = 10
int b = 3

int sum = a + b       # 13
int diff = a - b      # 7
int prod = a * b      # 30
int div = a / b       # 3 (Integer division because operands are int)
int pow = a ^ 2       # 100 (Power/Exponentiation)
```
🔹 Comparison Operators
These operations always evaluate to a bool value.

```Python
bool res1 = (a == b)  # false (Equal to)
bool res2 = (a != b)  # true  (Not equal to)
bool res3 = (a > b)   # true  (Greater than)
bool res4 = (a <= b)  # false (Less than or equal to)
```
🔹 Logical Operators
Uses clean English lowercase keywords instead of symbols to maximize readability.

```Python
bool x = true
bool y = false

bool and_res = x and y   # false
bool or_res  = x or y    # true
bool not_res = not x     # false
```

3. Explicit Type Conversion
The compiler will never guess your types or perform automatic casting (No Implicit Casting). Type conversion must be called explicitly using built-in functions.

```Python
string str_num = "100"
int native_num = int(str_num)      # Converts string -> int

int code = 65
string str_code = string(code)     # Converts int -> string

int score = 85
float f_score = float(score)       # Converts int -> float
```

4. Conditionals (if-elif-else)
np-lang uses indentation to define code blocks. Conditions do not require parentheses (), but must end with a colon :.

```Python
int score = 75

if score >= 80:
    print("Grade A")
elif score >= 70:
    print("Grade B")
elif score >= 60:
    print("Grade C")
else:
    print("Grade F")
```
5. Loops
🔹 While Loop
Runs continuously as long as the conditional expression remains true.

```Python
int count = 1

while count <= 5:
    print(count)
    count = count + 1
```

🔹 For Loop (Range-based)
Uses the in range(start, end) structure for clean iterative sequencing. The loop executes up to end - 1.

```Python
# Iterates from 0 to 4 (runs 5 times)
for i in range(0, 5):
    print(i)
```

6. Functions
Functions are declared using the fn keyword, followed by parameter inputs with explicit types. The Return Type is specified at the end using the arrow syntax -> right before the colon :.

🔹 Functions without a Return Value (Void)
If the function doesn't return anything, omit the arrow syntax.

```Python
fn say_hello(string name):
    print("Hello " + name)

# Invocation
say_hello("Mister NP")
```
🔹 Functions with a Return Value
Explicitly define -> Type before the block opens.

```Python
fn add_numbers(int x, int y) -> int:
    return x + y

fn check_adult(int age) -> bool:
    if age >= 20:
        return true
    return false

# Invocation
int result = add_numbers(5, 10)
bool is_adult = check_adult(23)
```
An language support multiple output function

```Python
(Remainder)
fn divide(int dividend, int divisor) -> int, int:
    int quotient = dividend / divisor
    int remainder = dividend % divisor
    return quotient, remainder

int q, int r = divide(10, 3)
print(q)  # Output: 3
print(r)  # Output: 1
```