---
title: BHCF Developer Quickstart Guide
status: Draft
owner: Team
last_updated: '2024-01-01'
tags:
- doc
see_also:
- none
---
# BHCF Developer Quickstart Guide

This guide walks through creating an HMEM object, mapping it to the CPU, creating a Tensor using that memory, creating a zero-copy tensor view, and properly tearing it down.

## Lifecycle Overview

```text
Create HMEM
   ↓
Map it
   ↓
Populate it
   ↓
Create Tensor referencing HMEM
   ↓
Create Tensor View
   ↓
Sync when required
   ↓
Consume it
   ↓
Destroy/release
```

## 1. Create HMEM and Map to CPU

We'll start by allocating a basic heterogeneous memory object and mapping it so the CPU can write to it.

```c
#include <bharat/hmem.h>
#include <bharat/compute/tensor.h>
#include <stdio.h>

int main() {
    bharat_hmem_handle_t hmem;
    void *cpu_ptr = NULL;

    // 1. Define HMEM descriptor
    bharat_hmem_desc_v1_t hmem_desc = {
        .version = BH_HMEM_DESC_VERSION_1,
        .struct_size = sizeof(bharat_hmem_desc_v1_t),
        .size = 1024 * 1024, // 1 MB
        .alignment = 4096,
        .usage_flags = BH_HMEM_USAGE_CPU_READ | BH_HMEM_USAGE_CPU_WRITE | BH_HMEM_USAGE_DEVICE_READ,
        .property_flags = BH_HMEM_PROP_CPU_COHERENT,
        .preferred_domain = BH_HMEM_DOMAIN_SYSTEM
    };

    // 2. Create the HMEM object
    bh_status_t status = bh_hmem_create(&hmem_desc, &hmem);
    if (status != BH_OK) return -1;

    // 3. Map it into the CPU virtual address space
    status = bh_hmem_map_cpu(hmem, BH_HMEM_ACCESS_READ | BH_HMEM_ACCESS_WRITE, &cpu_ptr);
    if (status != BH_OK) {
        bh_hmem_destroy(hmem);
        return -1;
    }

    // 4. Populate memory via the CPU pointer
    float *data = (float *)cpu_ptr;
    data[0] = 1.0f;
    // ...
```

## 2. Create Tensor Referencing HMEM

Now we'll define a tensor describing the data and bind it to the HMEM handle we just populated.

```c
    bh_tensor_t image;

    // Describe a 1x3x224x224 float32 tensor
    bh_tensor_desc_t tensor_desc = {
        .dtype = BH_DTYPE_F32,
        .rank = 4,
        .shape = {1, 3, 224, 224},
        .stride = {3*224*224, 224*224, 224, 1}, // Example dense strides
        .layout = BH_TENSOR_LAYOUT_DENSE,
        .hmem_property_flags = 0 // Inherits from HMEM
    };

    // Note: The SDK currently creates its own underlying HMEM via bh_tensor_create.
    // To explicitly bind an existing HMEM to a tensor structure directly bypassing allocation
    // (if not natively supported via bh_tensor_create right now), you build the tensor
    // metadata and set `tensor.memory = hmem`.
    // In actual implementation:
    // status = bh_tensor_create(&tensor_desc, &image);
    // image.memory = hmem;
```

```text
Tensor:
    1 × 3 × 224 × 224
    FP32
         │
         ▼
HMEM:
    backing memory object
```

## 3. Create a Tensor View

You can create a zero-copy sub-view of the same tensor. The underlying data does not move.

```c
    bh_tensor_t image_view;

    bh_tensor_view_desc_t view_desc = {
        .rank = 4,
        .layout = BH_TENSOR_LAYOUT_STRIDED,
        .shape = {1, 3, 128, 128}, // Smaller crop view
        .stride = image.stride,
        .byte_offset = 0 // Assuming top-left crop
    };

    status = bh_tensor_view(&image, &view_desc, &image_view);
    if (status != BH_OK) {
        // Handle error
    }
```

```text
Tensor: [1, 3, 1024, 1024]
         │
         ├── View A [1,3,256,256]
         ├── View B [1,3,256,256]
         └── View C [1,3,256,256]

All reference the same HMEM. Shape changes, stride changes, offset changes, but underlying data need not move.
```

## 4. Syncing and Cleanup

If the device will read this memory (and the memory isn't fully CPU-coherent), you must sync before consumption.

```c
    // Sync for the accelerator
    bh_hmem_sync_for_device(hmem, 0, 1024 * 1024);

    // Consume it (e.g. submit to accelerator) ...
    // Wait for completion ...

    // Cleanup
    bh_hmem_unmap_cpu(hmem);
    bh_hmem_destroy(hmem);
    // (If the tensor owned the memory, bh_tensor_destroy(&image) would clean it up)

    return 0;
}
```
