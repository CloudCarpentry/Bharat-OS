#include <assert.h>
#include <stddef.h>
#include <bharat/kernel/ds/bh_rcu.h>
#include <kernel/status.h>

static int callback_count = 0;
void test_callback(void *arg) {
    (void)arg;
    callback_count++;
}

void test_rcu_stub() {
    bh_rcu_read_lock();
    bh_rcu_read_unlock();
    bh_rcu_synchronize();
    assert(bh_rcu_current_epoch() > 0);

    assert(bh_rcu_call(test_callback, NULL) == K_OK);
    assert(callback_count == 1); // Stub invokes immediately
}

int main() {
    test_rcu_stub();
    return 0;
}
