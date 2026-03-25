# Scheduler Invariants & Host Testing Contract

The scheduler in Bharat-OS dictates the execution rules and state transitions for multikernel threading. Host tests rely on explicitly verifying these invariants without the complexity of hardware traps, MMU context switching, or SMP locking.

## Runqueue Invariants

- **`runnable_count` Definition:** The `runqueue.runnable_count` strictly counts the number of threads currently **residing** in the runqueue (e.g. the CFS red-black tree or priority buckets). It **does not** count the currently executing thread (`sched_current_thread`).
- **Thread Creation:** When a thread is created via `thread_create()`, it defaults to `THREAD_STATE_READY` and is implicitly enqueued onto the scheduler's runqueue, incrementing the `runnable_count`.
- **Idle Task:** The per-core `idle_thread` handles execution when no other tasks are runnable. It is usually not counted in the `runnable_count` as it bypassing normal enqueueing.

## Create / Enqueue Semantics

1. `thread_create(parent, entry)` allocates a thread slot, sets up the initial context, and enqueues the thread.
2. `thread_create_detached(parent, entry)` allocates and sets up the thread but leaves it un-enqueued, requiring the caller to manually call `sched_enqueue()`.
3. State transitions strictly flow through `CREATED -> READY -> RUNNING -> BLOCKED -> READY -> EXIT`.

## Host Test Fixture Model

Host tests mock the underlying CPU and timer architecture but validate the algorithmic logic of the scheduler.

### Testing Rules

- **Avoid Double Counting:** If a host test uses `thread_create()`, the thread is automatically placed on the runqueue. Do not manually `sched_enqueue()` it again without first removing it (e.g. via `sched_dequeue_task_l0()`).
- **Runqueue Drain:** Before testing specific priority picking logic, drain the initial monitor or idle tasks injected by `sched_init()`.
- **Verification:** Always verify that `runnable_count` correctly reflects the number of threads *waiting* to run, and that `sched_pick_next_ready()` removes the chosen thread from the queue, decrementing the count.

### Profile-Specific Deviations

- **GP / CLOUD Profile:** Uses fair scheduling (CFS-like `cfs_runqueue`), meaning bucket/count assumptions change. Ensure tests accurately reflect red-black tree dequeueing.
- **RT Profile:** Strict priority deadline aware. Minimal lazy state transitions.

## Anti-Patterns

- **Do not blindly weaken assertions.** If a test like `host_test_sched` fails due to `runnable_count == 1` when `2` is expected, it usually points to a logic drift in test setup (e.g., an automatic enqueue followed by a manual re-enqueue). Fix the test setup, do not change the assertion.
- **Do not manually modify `runnable_count`.** Let `sched_enqueue` and `sched_dequeue` handle the internal bookkeeping.
