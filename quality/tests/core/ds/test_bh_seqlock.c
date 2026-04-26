#include <assert.h>
#include <bharat/kernel/ds/bh_seqlock.h>

void test_seqlock_basic() {
    bh_seqlock_t lock;
    bh_seqlock_init(&lock);

    uint64_t seq = bh_seqlock_read_begin(&lock);
    // read data...
    assert(bh_seqlock_read_retry(&lock, seq) == false);

    bh_seqlock_write_begin(&lock);
    // update data...
    assert(bh_seqlock_read_retry(&lock, seq) == true);
    bh_seqlock_write_end(&lock);

    seq = bh_seqlock_read_begin(&lock);
    assert(bh_seqlock_read_retry(&lock, seq) == false);
}

int main() {
    test_seqlock_basic();
    return 0;
}
