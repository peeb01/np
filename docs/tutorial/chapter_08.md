# Chapter 8: High-Precision Numerics and Large Projects

This final chapter details NP's advanced numeric types, code modularization techniques, and memory management.

---

## 1. High-Precision Integers (int128 and int256)

NP supports high-precision integers native to the compiler, allowing you to bypass typical 64-bit limits:

### int128
Maps directly to GCC's native `__int128` signed type:
```python
int128 big1 = 123456789012345678901234567890
int128 big2 = 2
int128 res = big1 * big2
print("128-bit multiplication result:", res)
```

### int256
A software-implemented 256-bit signed integer for heavy numeric applications, cryptography, or high-precision math:
```python
int256 v256 = 100000000000000000000000000000000000000000000000000000000000
print("256-bit division (v256 / 3):", v256 / 3)
```

---

## 2. Multi-File Projects (Import System)

As your codebase grows, you can modularize your code by splitting functions, variables, and structs into external helper files and loading them using `import`:

Given a file `helper.np`:
```python
fn calculate_area(int w, int h) -> int:
    return w * h
```

In your main program:
```python
import "helper.np"

int area = calculate_area(10, 20)
print("Area:", area)
```
*NP features circular dependency protection, preventing errors if helper files import each other.*

---

## 3. Automatic Reference Counting (RAII)

NP manages memory automatically using C++ standard smart pointers (`std::shared_ptr`) to perform Reference Counting:

*   **Zero Leakage:** When an array or dictionary falls out of scope or is no longer referenced, the runtime automatically destructs it and reclaims its heap memory instantly and deterministically.
*   **Safety & Performance:** You get the safety of automatic memory management with zero runtime collection pauses, keeping execution speeds clean and predictable without JIT interpreter overhead.

---

## Conclusion

Congratulations! You have completed the NP Programming tutorial series. You are now equipped to build high-performance, memory-safe, and highly structured applications using NP.

[<- Back to Chapter 7](chapter_07.md) | [<- Back to Table of Contents](README.md)
