#include <bharat/kernel/ds/bh_rcu.h>
#include <kernel/status.h>

/* Baseline RCU stub implementation */

void bh_rcu_read_lock(void) {
    /* Baseline stub: no-op in non-preemptive kernel */
}

void bh_rcu_read_unlock(void) {
    /* Baseline stub: no-op in non-preemptive kernel */
}

bh_rcu_epoch_t bh_rcu_current_epoch(void) {
    /* Baseline stub: return constant epoch 1 */
    return 1;
}

void bh_rcu_synchronize(void) {
    /* Baseline stub: synchronize is immediate in non-preemptive/UP environment */
}

kstatus_t bh_rcu_call(bh_rcu_callback_t callback, void *arg) {
    /* Baseline stub: invoke callback immediately for now */
    if (callback) {
        callback(arg);
    }
    return K_OK;
}
