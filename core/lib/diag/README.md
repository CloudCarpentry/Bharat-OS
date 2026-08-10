<!-- SPDX-License-Identifier: MIT -->
# Bounded diagnostic mechanism

`bharat_diag` is an allocation-free, non-blocking SPSC mechanism intended for one ring per core. The writer drops the newest record when full, increments `dropped`, and preserves unread evidence. It never prints, panics, waits, or controls correctness. Caller-owned storage and payload memory must be valid for the duration of each call. Reset requires producer and consumer quiescence.

Release publication and acquire observation prevent a reader from seeing a partially copied record. Sequence and committed values detect stale/corrupt slots. `BHARAT_DIAG_ENABLED=OFF` makes `bh_diag_emit` return `BH_ERR_NOT_SUPPORTED` without touching the sink.
