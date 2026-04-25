# Algorithm Improvement Code Audit (Validated Against Current Tree)

This audit cross-checks the gap-analysis notes in this folder against the *current* codebase and lists the concrete algorithmic hotspots still worth improving.

## High-priority algorithm hotspots

### 1) Scheduler TID lookup remains linear in hot paths
- **Code reality:** `sched_find_thread_slot_by_tid_local()` still scans `SCHED_MAX_THREADS` linearly for each lookup.
- **Why it matters:** This function is used by multiple scheduling and IPC paths, so worst-case cost grows with thread-capacity rather than active thread count.
- **Where to improve:**
  - `core/kernel/src/sched/sched.c` (`sched_find_thread_slot_by_tid_local`)
  - callers in scheduler and IPC paths should use a faster index once introduced.
- **Suggested change:** Add a per-runqueue `tid -> slot_index` hash table (open addressing) plus generation checks; keep the current scan as debug fallback.

### 2) Netstack RX/TX path still performs avoidable packet copies
- **Code reality:** the virtio adapter copies packets from `packet_buf_t` to `netbuf` on RX and from `netbuf` back to `packet_buf_t` on TX.
- **Why it matters:** this adds two O(n) memory copies in the fast path even when no transformation is needed.
- **Where to improve:**
  - `core/services/netstack/src/driver_virtio_adapter.c`
- **Suggested change:** introduce a zero-copy wrapper path (shared buffer ownership + refcount handoff) between `packet_buf_t` and `netbuf_t`.

### 3) User/runtime memcpy/memset implementations are still scalar loops
- **Code reality:** kernel base string APIs already dispatch via `arch_memcpy/arch_memset`, but user/runtime libraries still use byte loops.
- **Why it matters:** service-level IPC/network code will remain limited by scalar copies even if kernel memops are optimized.
- **Where to improve:**
  - `lib/runtime/src/freestanding_string.c`
  - `lib/string/string.c`
- **Suggested change:** introduce architecture-specialized variants selected by build target (or compiler builtins for large copies) while preserving deterministic fallbacks.

### 4) ARP cache uses linear search and simplistic eviction
- **Code reality:** ARP cache is fixed-size (`ARP_CACHE_SIZE = 16`) with linear scan; eviction is now LRU-style (monotonic recency) instead of fixed-index replacement.
- **Why it matters:** lookup and update costs are O(n), and eviction quality is poor under churn.
- **Where to improve:**
  - `core/services/netstack/src/arp.c`
- **Suggested change:** next step is a small hash table to remove linear scan cost; LRU/timestamp eviction baseline is now in place.

### 5) NUMA migration tracking uses chained hash tables without synchronization
- **Code reality:** NUMA access/page-node tracking already moved beyond a simple list to hash buckets, but mutation and traversal are unsynchronized.
- **Why it matters:** when called concurrently (multi-core page access accounting), list corruption and inaccurate counts are possible.
- **Where to improve:**
  - `core/kernel/src/mm/pmm/numa.c`
- **Suggested change:** add per-bucket locks (or RCU-style reads + locked writes) and bounded bucket depth monitoring.

## Medium-priority improvements

### 6) Message CRC backend can be faster on non-ARM builds
- **Code reality:** dispatcher intentionally avoids x86 SSE4.2 because it is CRC32C (different polynomial), so non-ARM currently uses generic table CRC.
- **Why it matters:** correctness is right, but throughput can be improved with slicing-by-8/16 or PCLMUL-based IEEE CRC.
- **Where to improve:**
  - `lib/msg/crc.c`
  - `lib/msg/crc_generic.c`
- **Suggested change:** keep polynomial compatibility while adding higher-throughput IEEE CRC implementations.

### 7) Crypto service dispatch exists, but all handlers are stubs
- **Code reality:** opcodes are routed correctly, but all handlers return `CRYPTO_STATUS_ERR_NOT_IMPL`.
- **Why it matters:** all crypto requests currently fail, and no offload/software backend selection exists yet.
- **Where to improve:**
  - `core/services/security/crypto/dispatch.c`
- **Suggested change:** implement provider registry + capability-based backend selection (HW offload first, constant-time software fallback).

## Already improved versus the older gap notes

1. **ZSwap hashing is no longer naive modulus.**
   - Current implementation uses a 64-bit mix function and power-of-two mask indexing.
2. **IPv4/ICMP software checksum verification can be skipped when RX checksum flags are set.**
   - Netstack now checks RX checksum metadata flags before doing software verification.
3. **Kernel memory ops are not simple byte loops in the base API path.**
   - Kernel `memcpy/memset/memmove` route through architecture memops abstraction.
4. **NUMA mapping is not “simple list only” anymore.**
   - Current code uses hash tables for access/page-node tracking.

## Recommended execution order

1. **Scheduler TID index** (highest impact on core kernel hot path).
2. **Netstack zero-copy bridging** (large throughput win).
3. **Service/runtime memops optimization** (broad reduction in copy cost).
4. **Thread-safe NUMA accounting** (correctness + scalability).
5. **CRC throughput upgrade preserving IEEE polynomial correctness**.
6. **Crypto provider implementation and offload selection**.

## Implemented in this update (small scoped task)

- **ARP cache eviction quality improved** in `core/services/netstack/src/arp.c`:
  - Added monotonic `last_used` tracking per entry.
  - Switched full-cache replacement from fixed slot (`index 0`) to least-recently-used entry.
  - Updated cache hits to refresh recency.
- **Impact:** better stability under ARP churn with no interface/ABI changes; lookup complexity remains O(n) pending hash-index follow-up.
