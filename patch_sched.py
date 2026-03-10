import sys

def apply_patch(filename):
    with open(filename, "r") as f:
        content = f.read()

    # Make sched_stub match
    s1 = """typedef enum {
    THREAD_STATE_READY,
    THREAD_STATE_RUNNING,
    THREAD_STATE_BLOCKED,
    THREAD_STATE_SLEEPING,
    THREAD_STATE_TERMINATED
} thread_state_t;"""
    r1 = """typedef enum {
    THREAD_STATE_READY,
    THREAD_STATE_RUNNING,
    THREAD_STATE_BLOCKED,
    THREAD_STATE_SLEEPING,
    THREAD_STATE_TERMINATED,
    THREAD_STATE_DEG_PENDING
} thread_state_t;"""
    if s1 in content:
        content = content.replace(s1, r1)

    with open(filename, "w") as f:
        f.write(content)
