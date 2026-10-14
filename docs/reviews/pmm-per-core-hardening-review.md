---
title: PMM Per-Core Hardening Review
status: Completed / Archived
owner: Documentation Working Group
last_updated: 2026-10-14
tags:
  - docs
  - reviews
see_also:
  - README.md
---
# PMM Per-Core Hardening Review

> **Status:** **COMPLETED / ARCHIVED** (October 14, 2026)  
> **Resolution:** Implemented via `BHARAT_MAX_CPUS`, runtime active core validation, MPSC remote-free inbox, and per-core pcache magazines.

## Improvements
- Replaced hardcoded CPU limits with `BHARAT_MAX_CPUS`.
- Added runtime active core validation.
- Formalized remote-free enqueue path.
- Added per-core statistics.

## Testing
- Host tests cover local/remote free paths and overflow behavior.
