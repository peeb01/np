# Chapter 6: Custom Data Models with Structs

When working with complex data schemas, grouping properties under named structures makes your program much easier to manage. Structs in NP allow you to create custom, named data types.

---

## 1. Defining a Struct

A Struct declaration lists the names and types of its inner fields:

```python
struct User:
    string name
    int age
    bool is_admin
```

*Note: Struct instances are converted to dictionary-backed raw structures managed under `np_var` in C++, guaranteeing memory efficiency and compilation speed.*

---

## 2. Instantiating Structs

To create a new instance of a struct, call its constructor name and pass arguments matching the field types in order:

```python
User u = User("Bob", 25, true)
```

---

## 3. Accessing and Mutating Fields

NP supports accessing and mutating fields using the dot (`.`) notation:

```python
# Accessing fields
print("User Name:", u.name)       # -> Bob
print("Is Admin:", u.is_admin)    # -> true

# Mutating fields
u.age = 26
print("Updated Age:", u.age)      # -> 26
```

---

## 4. Nested Collections

You can nest struct instances inside arrays, dictionaries, or inside other structures:

```python
User u1 = User("Alice", 30, false)
User u2 = User("Bob", 28, true)

array users = [u1, u2]

for u in users:
    print("User:", u.name, "Age:", u.age)
```

---

[<- Back to Chapter 5](chapter_05.md) | [Next: Chapter 7 - File Operations and Exceptions ->](chapter_07.md)
