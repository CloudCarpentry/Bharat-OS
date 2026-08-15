---
title: "ADR-026: Native monotonic time path"
status: Accepted
owner: Kernel ABI Maintainers
last_updated: 2026-08-13
---

# ADR-026: Native monotonic time path

## Context

The kernel has one canonical monotonic nanosecond source, `bh_ktime_now()`, but
the native BharatLibC backend did not expose it and kernel benchmarks could use
the build host's `clock_gettime()`. That made benchmark provenance ambiguous and
left target `clock_gettime(CLOCK_MONOTONIC)` unsupported.

## Decision

Native syscall 24, `BH_SYS_TIME_GET`, copies one fixed-width `uint64_t`
monotonic-nanosecond value to userspace. It accepts only
`BH_CLOCK_MONOTONIC`; realtime is not manufactured. The handler obtains time
exclusively from `bh_ktime_now()` and therefore retains its fail-closed panic
when the HAL clock is unavailable. BharatLibC's Bharat backend converts the
nanosecond value to `bh_bsys_timespec_t`, and the existing POSIX adapter exposes
that through `clock_gettime()`. The Linux personality translates Linux
`clock_gettime(CLOCK_MONOTONIC)` independently at its compatibility boundary.

Kernel benchmarks now call `bh_ktime_now()` directly. Host-mode BharatLibC
tests may still use the host backend, but cannot be confused with target kernel
benchmark evidence.

The diagnostic control plane exposes `timer info` as a production-safe query
and `timer test` as a development-only, diagnostic-capability-gated query. The
diagnostic service response for `timer info` must identify clock source and
resolution. `timer test` must report monotonicity, cross-core skew, deadline
programming accuracy, interrupt jitter, and scheduler wakeup latency; a backend
that cannot measure a field must report it unavailable rather than synthesize a
value.

## Consequences

The ABI grows append-only without changing existing numbers. User pointers are
written only through fault-safe usercopy. No capability is required to read the
non-secret monotonic clock. Realtime remains unsupported until a service owns a
wall-clock policy. Timer tests remain service-orchestrated policy and do not
move benchmark orchestration into the kernel.
