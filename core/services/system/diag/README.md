<!-- SPDX-License-Identifier: MIT -->
# Diagnostic collector

This compile-safe service core drains independent SPSC rings, rejects malformed or unsupported records, detects sequence gaps, produces a generic health snapshot, and delegates JSON Lines output to a caller-provided bounded writer. It is intentionally not connected to init, boot, service supervision, persistence, networking, or UI policy.
