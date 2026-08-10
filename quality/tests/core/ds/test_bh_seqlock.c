#include <assert.h>
#include <bharat/kernel/ds/bh_seqlock.h>
#include <atomic.h>

void test_seqlock_basic() {
    bh_seqlock_t lock;
    bh_seqlock_init(&lock);

    uint32_t seq = bh_seqlock_read_begin(&lock);
    assert((seq & 1) == 0); // must be even initially
    assert(bh_seqlock_read_retry(&lock, seq) == false);

    bh_seqlock_write_begin(&lock);
    assert(bh_seqlock_read_retry(&lock, seq) == true);
    bh_seqlock_write_end(&lock);

    seq = bh_seqlock_read_begin(&lock);
    assert((seq & 1) == 0); // must be even after write
    assert(bh_seqlock_read_retry(&lock, seq) == false);
}

void test_seqlock_wrap() {
    bh_seqlock_t lock;
    bh_seqlock_init(&lock);

    // Artificially push sequence near wrap point
    atomic32_set(&lock.sequence, 0xFFFFFFFC);

    // Writer increments 1: FFFFFFFC -> FFFFFFFD -> FFFFFFFE
    bh_seqlock_write_begin(&lock);
    bh_seqlock_write_end(&lock);

    uint32_t seq = bh_seqlock_read_begin(&lock);
    assert(seq == 0xFFFFFFFE);

    // Writer increments 2: FFFFFFFE -> FFFFFFFF -> 00000000
    bh_seqlock_write_begin(&lock);
    bh_seqlock_write_end(&lock);

    assert(bh_seqlock_read_retry(&lock, seq) == true);

    seq = bh_seqlock_read_begin(&lock);
    assert(seq == 0); // Wrap occurred safely!
    assert(bh_seqlock_read_retry(&lock, seq) == false);
}

int main() {
    test_seqlock_basic();
    test_seqlock_wrap();
    return 0;
}
