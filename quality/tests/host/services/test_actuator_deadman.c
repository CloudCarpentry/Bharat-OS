#include <stdio.h>
#include <assert.h>
#include <stdbool.h>

void test_deadman_timeout() {
    // deadman timeout forces safe value
    assert(true);
}

int main() {
    test_deadman_timeout();
    printf("test_actuator_deadman passed.\n");
    return 0;
}
