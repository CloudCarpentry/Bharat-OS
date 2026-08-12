---
title: BHCF-002 Tensor SDK
status: Experimental P0
owner: SDK and Compute Runtime Teams
last_updated: 2026-08-12
---
# BHCF-002: Tensor SDK

> Tensor is an SDK/runtime semantic object. HMEM is the kernel memory mechanism.

`bh_tensor_t` is public metadata: an opaque HMEM handle, dtype, rank (maximum eight), shape, byte strides, byte range, layout, and ownership flags. It contains no kernel, HAL, driver, physical, GPU, or NPU pointer. F32/F16/BF16, signed I8/I16/I32, U8, and BOOL have explicitly defined element widths.

Rank zero is a scalar with one element. All non-scalar dimensions must be nonzero. Validation uses checked multiplication, validates dense or explicit byte strides, checks range addition for overflow, queries HMEM size, and rejects a range outside that object. Dense tensors receive canonical row-major byte strides. Strided tensors permit non-contiguous storage but no stride smaller than one element.

An owning tensor creates and destroys its HMEM. A view shares the parent's handle, changes only shape/stride/range, and never destroys storage. Applications must keep the owning tensor alive for every view; P0 deliberately does not introduce an unsafe implicit reference. Future runtime integration will adapt the existing tensor dispatcher and Virtual NPU backend to this representation rather than create another executor.

Backend selection, graph operations, placement, queues, and scheduling remain runtime/service policy. A later compute queue can consume the same HMEM without changing tensor semantics.
