<!-- SPDX-License-Identifier: MIT -->
# Observability integration roadmap

This phase supplies contracts, per-owner rings, an unwired collector core, and host evidence tools. After userspace boot stabilizes, owners may instantiate per-core rings and emit catalogued stages using HAL monotonic time. Later service-supervisor integration must add capability authorization, bounded IPC transport, and privacy policy. Health consumers, including UI, consume snapshots only after that service boundary exists; they must not read kernel ring storage directly.
