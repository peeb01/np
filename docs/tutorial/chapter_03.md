# Chapter 3: Control Flow (Conditionals and Loops)

NP uses Python-style block syntax. Blocks are defined using a colon (`:`) and indentation (4 spaces or tabs), rather than curly braces.

---

## 1. Decision Making (if, elif, else)

You can direct program flow using conditionals combined with logical comparison operators (such as `>`, `<`, `==`, `!=`, `>=`, `<=`).

```python
int score = 75

if score >= 80:
    print("Grade A")
elif score >= 70:
    print("Grade B")
elif score >= 60:
    print("Grade C")
else:
    print("Under review")
```

---

## 2. While Loops

A `while` loop repeatedly executes its block as long as its condition evaluates to `true`:

```python
int x = 1
while x <= 5:
    print("Iteration:", x)
    x = x + 1
```

---

## 3. For Loops

In NP, `for` loops serve two distinct roles: iterating over a range of numbers, or iterating over a collection's elements.

### Iterating Over a Range
Use `range(start, end)` to scan values starting from `start` (inclusive) up to `end` (exclusive):

```python
# Prints values from 1 to 4
for i in range(1, 5):
    print("i =", i)
```

### Iterating Over Collections (Foreach)
You can iterate directly over elements of arrays or other collections using `for var in collection:`:

```python
array countries = ["Thailand", "Japan", "Singapore"]

for country in countries:
    print("Country:", country)
```

---

[<- Back to Chapter 2](chapter_02.md) | [Next: Chapter 4 - Functions and Scoping Rules ->](chapter_04.md)
