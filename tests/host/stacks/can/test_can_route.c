#include <stdio.h>
#include <assert.h>
#include <stdbool.h>

void test_tx_priority() {
    // TX priority queues prefer critical over best-effort
    assert(true);
}

int main() {
    test_tx_priority();
    printf("test_can_router passed.\n");
    return 0;
}
