#include <bharat/kernel/ds/bh_seqlock.h>

void bh_seqlock_init(bh_seqlock_t *lock) {
    atomic32_set(&lock->sequence, 0);
    spin_lock_init(&lock->writer_lock);
}

uint32_t bh_seqlock_read_begin(const bh_seqlock_t *lock) {
    uint32_t seq;
    do {
        seq = atomic32_load_acquire(&lock->sequence);
        /* Ensure sequence is even (no write in progress).
         * If odd, spin and wait for the writer to finish.
         */
        if (seq & 1) {
            arch_cpu_relax();
        }
    } while (seq & 1);

    // Acquire barrier already provided by atomic32_load_acquire
    return seq;
}

bool bh_seqlock_read_retry(const bh_seqlock_t *lock, uint32_t seq) {
    // Enforce ordering: protected data reads must complete before reading sequence again.
    smp_rmb();
    uint32_t current = atomic32_load_acquire(&lock->sequence);

    /* If sequence has changed or is odd, retry */
    return (current != seq) || (current & 1);
}

void bh_seqlock_write_begin(bh_seqlock_t *lock) {
    spin_lock(&lock->writer_lock);

    uint32_t seq = atomic32_load_relaxed(&lock->sequence);
    /* Increment to odd to signal write in progress */
    seq++;
    atomic32_store_relaxed(&lock->sequence, seq);

    /* Ensure odd publication is ordered before protected-data stores */
    smp_wmb();
}

void bh_seqlock_write_end(bh_seqlock_t *lock) {
    /* Ensure protected-data stores are published before bumping sequence to even */
    smp_wmb();

    uint32_t seq = atomic32_load_relaxed(&lock->sequence);
    /* Increment to even to signal write finished */
    seq++;
    atomic32_store_release(&lock->sequence, seq);

    spin_unlock(&lock->writer_lock);
}
