# Chapter 5: Collections (Array, Dictionary) and Slicing

This chapter explores working with collection data types, querying them, extracting subsets, and processing element operations.

---

## 1. Arrays and Built-in Methods

An Array is an ordered list of elements accessible by a zero-based index:

```python
array my_arr = [50, 10, 40, 20, 30]

# Index access
print("First element:", my_arr[0])  # -> 50
```

### Collection Methods:
*   `sort()`: Sorts the array in place in ascending order.
*   `reverse()`: Reverses the array elements in place.
*   `contains(value)`: Returns `true` if the array contains the value, otherwise `false`.

```python
my_arr.sort()
print("Sorted:", my_arr)      # -> [10, 20, 30, 40, 50]

my_arr.reverse()
print("Reversed:", my_arr)    # -> [50, 40, 30, 20, 10]

print("Contains 30:", my_arr.contains(30))  # -> 1 (true)
```

---

## 2. Dictionaries

A Dictionary stores values as key-value pairs. You can read, write, and update data using string-based keys:

```python
dict grades = {"Alice": 85, "Bob": 92}

# Query key
print("Alice grade:", grades["Alice"])  # -> 85

# Update key value
grades["Bob"] = 95
```

---

## 3. Vectorized Slicing

NP supports slicing operations on arrays and strings using the `[start:end]` syntax. It supports negative indexes, which count elements backward from the end:

```python
array source = [10, 20, 30, 40, 50]

# Slice index 1 up to index before 4
print(source[1:4])  # -> [20, 30, 40]

# Omitted start: defaults to beginning
print(source[:3])   # -> [10, 20, 30]

# Omitted end: defaults to end
print(source[2:])   # -> [30, 40, 50]

# Negative index
print(source[-2:])  # -> [40, 50]
```

---

## 4. Vectorized Math and Broadcasting

You can perform arithmetic operations directly on arrays. NP executes these calculations element-by-character (element-wise) or broadcasts scalars:

```python
array A = [1, 2, 3]
array B = [10, 20, 30]

# Element-wise addition
print("A + B =", A + B)  # -> [11, 22, 33]

# Scalar broadcasting (multiplies every element by 5)
print("A * 5 =", A * 5)  # -> [5, 10, 15]
```

---

[<- Back to Chapter 4](chapter_04.md) | [Next: Chapter 6 - Custom Data Models with Structs ->](chapter_06.md)
