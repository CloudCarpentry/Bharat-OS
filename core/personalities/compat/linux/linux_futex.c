/* SPDX-License-Identifier: MIT */
#include "linux_futex.h"
#include "linux_errno.h"
#include "linux_types.h"
#include "syscall/usercopy.h"
#include "sched/sched.h"
#include "time/ktime.h"
#include "spinlock.h"
#include <stddef.h>
#include <stdbool.h>

#define FUTEX_BUCKETS 64

typedef struct futex_waiter {
    uintptr_t uaddr;
    uint64_t key;
    uint32_t bitset;
    bh_thread_t *thread;
    bool woken;
    bool unlinked;
    struct futex_waiter *next;
} futex_waiter_t;

typedef struct {
    spinlock_t lock;
    futex_waiter_t *head;
} futex_bucket_t;

static futex_bucket_t g_futex_buckets[FUTEX_BUCKETS];
static bool g_futex_initialized = false;

static void futex_init_buckets_if_needed(void) {
    if (__atomic_load_n(&g_futex_initialized, __ATOMIC_ACQUIRE)) {
        return;
    }
    for (int i = 0; i < FUTEX_BUCKETS; i++) {
        spin_lock_init(&g_futex_buckets[i].lock);
        g_futex_buckets[i].head = NULL;
    }
    __atomic_store_n(&g_futex_initialized, true, __ATOMIC_RELEASE);
}

static uint64_t futex_calc_key(uintptr_t uaddr, uint64_t pid, int op) {
    if (op & FUTEX_PRIVATE_FLAG) {
        return pid ^ (uint64_t)uaddr;
    }
    return (uint64_t)uaddr;
}

static uint32_t futex_hash(uint64_t key) {
    uint64_t v = (key >> 2) ^ (key >> 16);
    return (uint32_t)(v % FUTEX_BUCKETS);
}

bh_operation_result_t linux_futex_dispatch(bh_syscall_ctx_t *ctx) {
    if (!ctx) return bh_op_result_value(-LINUX_EINVAL);

    futex_init_buckets_if_needed();

    uintptr_t uaddr = ctx->regs.arg[0];
    int op = (int)ctx->regs.arg[1];
    uint32_t val = (uint32_t)ctx->regs.arg[2];
    uintptr_t timeout_ptr = ctx->regs.arg[3];
    uint32_t bitset = (uint32_t)ctx->regs.arg[5];

    int cmd = op & (int)FUTEX_CMD_MASK;
    uint64_t pid = ctx->process ? ctx->process->process_id : 0;

    if (uaddr & 3U) {
        return bh_op_result_value(-LINUX_EINVAL);
    }

    if (cmd == FUTEX_WAIT || cmd == FUTEX_WAIT_BITSET) {
        uint32_t cur = 0;
        bh_status_t st = bh_copy_from_user(&cur, (const void *)uaddr, sizeof(uint32_t));
        if (st != BH_OK) {
            return bh_op_result_value(-LINUX_EFAULT);
        }

        if (cur != val) {
            return bh_op_result_value(-LINUX_EAGAIN);
        }

        uint64_t sleep_ms = 0;
        if (timeout_ptr != 0) {
            linux_timespec_t ts = {0};
            st = bh_copy_from_user(&ts, (const void *)timeout_ptr, sizeof(ts));
            if (st != BH_OK) {
                return bh_op_result_value(-LINUX_EFAULT);
            }
            if (ts.tv_sec < 0 || ts.tv_nsec < 0 || ts.tv_nsec >= 1000000000L) {
                return bh_op_result_value(-LINUX_EINVAL);
            }
            sleep_ms = (uint64_t)ts.tv_sec * 1000U + (uint64_t)(ts.tv_nsec / 1000000L);
            if (sleep_ms == 0 && ts.tv_nsec > 0) {
                sleep_ms = 1;
            }
        }

        uint64_t key = futex_calc_key(uaddr, pid, op);
        uint32_t b_idx = futex_hash(key);
        futex_bucket_t *bucket = &g_futex_buckets[b_idx];

        spin_lock(&bucket->lock);

        /* Atomic verify under bucket lock to eliminate lost wakeups */
        st = bh_copy_from_user(&cur, (const void *)uaddr, sizeof(uint32_t));
        if (st != BH_OK) {
            spin_unlock(&bucket->lock);
            return bh_op_result_value(-LINUX_EFAULT);
        }
        if (cur != val) {
            spin_unlock(&bucket->lock);
            return bh_op_result_value(-LINUX_EAGAIN);
        }

        futex_waiter_t waiter;
        waiter.uaddr = uaddr;
        waiter.key = key;
        waiter.bitset = (cmd == FUTEX_WAIT_BITSET && bitset != 0) ? bitset : 0xFFFFFFFFU;
        waiter.thread = ctx->thread;
        waiter.woken = false;
        waiter.unlinked = false;
        waiter.next = bucket->head;
        bucket->head = &waiter;

        spin_unlock(&bucket->lock);

        if (sleep_ms > 0) {
            sched_sleep(sleep_ms);
        } else {
            sched_block();
        }

        /* Safely unlink waiter under bucket lock */
        spin_lock(&bucket->lock);
        if (!waiter.unlinked) {
            futex_waiter_t **curr = &bucket->head;
            while (*curr) {
                if (*curr == &waiter) {
                    *curr = waiter.next;
                    waiter.unlinked = true;
                    break;
                }
                curr = &(*curr)->next;
            }
        }
        bool was_woken = waiter.woken;
        spin_unlock(&bucket->lock);

        if (sleep_ms > 0 && !was_woken) {
            return bh_op_result_value(-LINUX_ETIMEDOUT);
        }

        return bh_op_result_value(0);
    } else if (cmd == FUTEX_WAKE || cmd == FUTEX_WAKE_BITSET) {
        if (val == 0) {
            return bh_op_result_value(0);
        }

        uint64_t key = futex_calc_key(uaddr, pid, op);
        uint32_t b_idx = futex_hash(key);
        futex_bucket_t *bucket = &g_futex_buckets[b_idx];
        uint32_t target_bitset = (cmd == FUTEX_WAKE_BITSET && bitset != 0) ? bitset : 0xFFFFFFFFU;
        int woken = 0;

        spin_lock(&bucket->lock);

        futex_waiter_t **curr = &bucket->head;
        while (*curr && (uint32_t)woken < val) {
            futex_waiter_t *w = *curr;
            if (w->uaddr == uaddr && w->key == key && (w->bitset & target_bitset)) {
                *curr = w->next;
                w->unlinked = true;
                w->woken = true;
                if (w->thread) {
                    sched_wakeup(w->thread);
                }
                woken++;
            } else {
                curr = &(*curr)->next;
            }
        }

        spin_unlock(&bucket->lock);

        return bh_op_result_value(woken);
    }

    return bh_op_result_value(-LINUX_ENOSYS);
}
