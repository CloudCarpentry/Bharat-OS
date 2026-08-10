<!-- SPDX-License-Identifier: MIT -->
# ADR-018: Bounded diagnostic evidence

- Status: Accepted
- Date: 2026-08-06

## Decision

Adopt a fixed-width version-1 event header, caller-owned SPSC rings intended one per core, drop-newest overflow, and a service-owned collector. Release/acquire publication preserves slot integrity without a global lock. Diagnostics remain optional and cannot panic or determine correctness.

## Consequences

Evidence survives temporary reader lag until capacity is exhausted, with explicit dropped counts. Payload size is bounded at build time. Integrating kernel producers later requires a capability-mediated export boundary and HAL monotonic time. Persisting or uploading evidence remains out of scope.
