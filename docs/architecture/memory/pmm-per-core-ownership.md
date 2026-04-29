# PMM Per-Core Ownership

## Overview
Bharat-OS PMM uses per-core magazines for order-0 pages to reduce lock contention and improve performance.

## Ownership Rules
- Each page has an `owner_core_id`.
- Pages are freed back to the owner's magazine when possible.
- Remote frees are enqueued in the owner's `remote_inbox`.

## Invariants
- Pinned pages cannot enter the per-core cache.
- Double-frees are rejected.
- Remote inbox overflows are handled by falling back to the global buddy allocator.

## Stats
Per-core stats track:
- Alloc hits/misses.
- Refills and drains.
- Remote enqueue counts and failures.
