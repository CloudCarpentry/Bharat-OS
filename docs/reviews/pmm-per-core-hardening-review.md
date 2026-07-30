---
title: PMM Per-Core Hardening Review
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - reviews
see_also:
  - README.md
---
# PMM Per-Core Hardening Review

## Improvements
- Replaced hardcoded CPU limits with `BHARAT_MAX_CPUS`.
- Added runtime active core validation.
- Formalized remote-free enqueue path.
- Added per-core statistics.

## Testing
- Host tests cover local/remote free paths and overflow behavior.
