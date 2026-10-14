/* SPDX-License-Identifier: MIT */
#include "linux_futex.h"
#include "linux_errno.h"
#include "syscall/usercopy.h"
#include "sched/sched.h"
#include "time/ktime.h"
#include <stddef.h>

#define FUTEX_BUCKETS 64

typedef struct futex_waiter {
    uintptr_t uaddr;
    uint64_t process_id;
    uint32_t bitset;
    bh_thread_t *thread;
    struct futex_waiter *next;
} futex_waiter_t;

typedef struct {
    futex_waiter_t *head;
} futex_bucket_t;

static futex_bucket_t g_futex_buckets[FUTEX_BUCKETS];

static uint32_t futex_hash(uintptr_t uaddr, uint64_t pid) {
    uintptr_t v = (uaddr >> 2) ^ (uintptr_t)pid;
    return (uint32_t)(v % FUTEX_BUCKETS);
}

#include "linux_types.h"

bh_operation_result_t linux_futex_dispatch(bh_syscall_ctx_t *ctx) {
    if (!ctx) return bh_op_result_value(-LINUX_EINVAL);

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

        uint32_t b_idx = futex_hash(uaddr, pid);
        futex_waiter_t waiter;
        waiter.uaddr = uaddr;
        waiter.process_id = pid;
        waiter.bitset = (cmd == FUTEX_WAIT_BITSET && bitset != 0) ? bitset : 0xFFFFFFFFU;
        waiter.thread = ctx->thread;
        waiter.next = g_futex_buckets[b_idx].head;
        g_futex_buckets[b_idx].head = &waiter;

        if (sleep_ms > 0) {
            sched_sleep(sleep_ms);
        } else {
            bh_thread_yield();
        }

        /* Unlink waiter */
        futex_waiter_t **curr = &g_futex_buckets[b_idx].head;
        while (*curr) {
            if (*curr == &waiter) {
                *curr = waiter.next;
                break;
            }
            curr = &(*curr)->next;
        }

        return bh_op_result_value(0);
    } else if (cmd == FUTEX_WAKE || cmd == FUTEX_WAKE_BITSET) {
        uint32_t b_idx = futex_hash(uaddr, pid);
        uint32_t target_bitset = (cmd == FUTEX_WAKE_BITSET && bitset != 0) ? bitset : 0xFFFFFFFFU;
        int woken = 0;

        futex_waiter_t **curr = &g_futex_buckets[b_idx].head;
        while (*curr && (val == 0 || (uint32_t)woken < val)) {
            futex_waiter_t *w = *curr;
            if (w->uaddr == uaddr && w->process_id == pid && (w->bitset & target_bitset)) {
                *curr = w->next;
                if (w->thread) {
                    sched_wakeup(w->thread);
                }
                woken++;
            } else {
                curr = &(*curr)->next;
            }
        }

        return bh_op_result_value(woken);
    }

    return bh_op_result_value(-LINUX_ENOSYS);
}
