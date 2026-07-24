# PMM Per-Core Hardening Review

## Improvements
- Replaced hardcoded CPU limits with `BHARAT_MAX_CPUS`.
- Added runtime active core validation.
- Formalized remote-free enqueue path.
- Added per-core statistics.

## Testing
- Host tests cover local/remote free paths and overflow behavior.
