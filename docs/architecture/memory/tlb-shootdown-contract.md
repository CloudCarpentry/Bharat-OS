# TLB Shootdown Contract

## Overview
This document defines the contract for TLB shootdowns in Bharat-OS. The shootdown protocol ensures that TLB invalidations are coordinated across multiple cores in a bounded and reliable manner.

## Request Lifecycle
Every remote TLB invalidation request has an explicit lifecycle:
1. **Allocation**: A tracking slot is allocated using `tlb_pending_alloc`.
2. **Dispatch**: The request is sent to target CPUs via URPC or legacy mailbox fallback.
3. **Acknowledgement**: Target CPUs execute the local flush and send an ACK.
4. **Completion**: The source CPU waits for all ACKs or a timeout.

## Failure Policies
Callers can specify a failure policy:
- `TLB_FAIL_RETURN_ERROR`: Return `K_ERR_TIMEOUT` to the caller.
- `TLB_FAIL_ISOLATE_ASPACE`: (Future) Mark the address space as unsafe.
- `TLB_FAIL_KERNEL_PANIC`: Panic the kernel to prevent memory corruption.

## Bounded Wait
The shootdown path uses a bounded wait loop with a configurable retry count and timeout. Unbounded spins are strictly forbidden.
