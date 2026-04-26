#include <bharat/kernel/ds/bh_seqlock.h>

void bh_seqlock_init(bh_seqlock_t *lock) {
    lock->sequence = 0;
}

uint64_t bh_seqlock_read_begin(const bh_seqlock_t *lock) {
    uint64_t seq;
    do {
        seq = lock->sequence;
        /* Ensure sequence is even (no write in progress) */
    } while (seq & 1);
    return seq;
}

bool bh_seqlock_read_retry(const bh_seqlock_t *lock, uint64_t seq) {
    /* If sequence has changed, retry */
    return (lock->sequence != seq);
}

void bh_seqlock_write_begin(bh_seqlock_t *lock) {
    /* Increment to odd to signal write in progress */
    lock->sequence++;
}

void bh_seqlock_write_end(bh_seqlock_t *lock) {
    /* Increment to even to signal write finished */
    lock->sequence++;
}
