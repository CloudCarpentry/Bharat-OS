# Scheduler Invariants and Host-Test Contract

## Purpose

Capture scheduler invariants that host tests should assert while keeping expectations aligned with actual runqueue semantics.

## Invariants

1. **Single ownership of runnable placement**
   - A runnable thread is owned by one core/runqueue representation at a time.
2. **Runnable count accuracy**
   - `runnable_count` tracks queued runnable threads, not necessarily the currently running thread.
3. **Bitmap/list consistency (priority mode)**
   - If a priority queue list is empty, its bitmap bit must be clear; non-empty list requires bit set.
4. **Policy-appropriate queue representation**
   - Priority policy uses per-priority lists/bitmap.
   - Cloud-fair uses CFS tree.
   - EDF uses deadline tree.
5. **Remote enqueue discipline**
   - Remote arrivals land in `pending_inbox` and are drained locally in reschedule path.

## Host test guidance

- Avoid double enqueue in fixtures (especially when using helper creation paths that already enqueue).
- Validate queue depth transitions around `enqueue`, `pick`, `dequeue`, `wake`, `sleep` operations.
- Assert policy-specific expectations only under the active policy (priority vs CFS vs EDF).
- For cross-core behavior, assert inbox draining effects rather than assuming direct foreign-runqueue mutation.

## Regression patterns to watch

- Runnable underflow/overflow from unmatched enqueue/dequeue.
- Bitmap/list drift in priority mode.
- Unexpected state transitions bypassing scheduler helpers.
- AI suggestion queue flooding without bounded handling behavior.
