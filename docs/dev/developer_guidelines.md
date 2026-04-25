---
title: Bharat-OS Developer Guidelines
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - dev
see_also:
  - README.md
---
# Bharat-OS Developer Guidelines

This document outlines the modern C guidelines for kernel and embedded development within the Bharat-OS project, focusing heavily on the **C23 standard (ISO/IEC 9899:2024)**. For embedded, OS, and kernel development, these features are essential for memory safety, compile-time optimization, and hardware-level bit manipulation.

---

## 1. Enhanced Bit Manipulation (`<stdbit.h>`)

Kernel and driver development spend a massive amount of time on bit-twiddling. The new `<stdbit.h>` standardizes these operations, removing the need for compiler-specific built-ins (like `__builtin_popcount`):

*   **`stdc_count_ones()` / `stdc_count_zeros()`:** Efficiently count set bits.
*   **`stdc_leading_zeros()` / `stdc_trailing_zeros()`:** Essential for priority encoders or finding the highest set bit in a register.
*   **`__STDC_ENDIAN_NATIVE__`:** Built-in macros to detect endianness at compile-time without complex build scripts.

---

## 2. Type-Safe Null Pointers (`nullptr`)

The old `NULL` macro is often just `#define NULL 0`, which can lead to bugs in variadic functions or `_Generic` selections where `0` is treated as an `int` rather than a pointer.

*   **Feature:** `nullptr` is a dedicated keyword of type `nullptr_t`.
*   **Benefit:** Prevents accidental integer-to-pointer conversions and makes kernel-level API debugging much clearer.

---

## 3. Compile-Time Constants (`constexpr`)

While C has always had macros, `constexpr` allows you to define typed constants that the compiler must evaluate at compile-time.

*   **Embedded Use Case:** Defining memory-mapped I/O (MMIO) offsets or fixed-point math tables. Unlike `const`, `constexpr` values can be used as array sizes or in other constant expressions without overhead.

---

## 4. Direct Binary Embedding (`#embed`)

This is a game-changer for firmware developers who need to include binary blobs (like FPGA bitstreams, firmwares, or certificates) directly into their C code.

*   **Before:** You used `xxd -i` or linker scripts to turn binary files into C arrays.
*   **Now:**
    ```c
    const uint8_t fw_blob[] = {
        #embed "firmware.bin"
    };
    ```

---

## 5. Bit-Precise Integers (`_BitInt(N)`)

Standard C types (`int`, `long`) are often too wide for specific hardware registers (like a 3-bit status field or a 12-bit ADC value).

*   **Feature:** `_BitInt(N)` allows you to define an integer of exactly `N` bits.
*   **Kernel/Embedded Benefit:** The compiler can optimize storage and arithmetic for the exact width of your hardware, potentially reducing register pressure.

---

## Sample Code: Modern C Hardware Driver Header

Here is how you might use these features in a modern driver:

```c
#include <stdint.h>
#include <stdbit.h>
#include <assert.h>

// 1. Using constexpr for hardware offsets
constexpr uint32_t CTRL_REG_OFFSET = 0x40;

// 2. Using _BitInt for exact hardware fields
typedef struct {
    unsigned _BitInt(3) priority; // Exactly 3 bits
    unsigned _BitInt(1) enabled;  // Exactly 1 bit
    unsigned _BitInt(12) address; // Exactly 12 bits
} DevConfig;

void process_interrupt(uint32_t status_reg) {
    // 3. Using stdbit.h to find the highest priority pending bit
    if (status_reg > 0) {
        int highest_bit = stdc_leading_zeros(status_reg);
        // Handle interrupt...
    }
}

// 4. Using nullptr for safer API checks
void init_device(DevConfig *config) {
    static_assert(sizeof(DevConfig) <= 4); // C23: static_assert doesn't need a message
    if (config == nullptr) return;
    // Initialization logic...
}
```

---

## AI/ML and Semiconductors: Advanced Applications

Given Bharat-OS's target across a wide device spectrum, including wearables and high-performance computation nodes, leveraging C23 for AI/ML and Semiconductor workloads is critical.

### 1. Bit-Precise Math

In modern AI, particularly quantized neural networks running on custom edge hardware, maintaining the exact bit precision is crucial. `_BitInt` allows us to natively implement quantized models (like 4-bit, 8-bit, or even odd configurations like 3-bit weights) without wasting registers or relying on manual packing and unpacking sequences.

*   **Use case:** Accelerating 4-bit or 8-bit quantized neural network inference directly in kernel space or within optimized drivers for AI coprocessors.
*   **Benefit:** Memory footprint for weights can be exactly matched to hardware architecture, and the compiler can optimize ALUs around these bit widths.

### 2. Memory Safety

For operations relying on cryptography or sensitive AI models in the kernel module, keys and sensitive intermediate buffers must be zeroed before releasing memory. A common vulnerability occurs when the compiler optimizes away `memset` calls on variables that are no longer used.

*   **Feature:** Use the C23 `memset_explicit()` function to clear sensitive keys.
*   **Benefit:** `memset_explicit()` explicitly tells the compiler that the zeroing operation must *not* be optimized away, ensuring memory safety and preventing information leakage in critical kernel spaces.
