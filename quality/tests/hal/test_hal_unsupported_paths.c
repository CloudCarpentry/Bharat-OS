#include <stdio.h>
#include <assert.h>
#include "hal/hal_capabilities.h"
#include "kernel/status.h"

// Forward declaration of the unsupported op helper
extern kstatus_t hal_unsupported_op(void);

void test_unsupported_op(void) {
    kstatus_t status = hal_unsupported_op();
    assert(status == K_ERR_UNSUPPORTED);
    printf("Unsupported operation returns K_ERR_UNSUPPORTED: PASSED\n");
}

int main(void) {
    printf("Starting HAL Unsupported Path Tests...\n");

    test_unsupported_op();

    printf("All HAL Unsupported Path Tests: PASSED\n");
    return 0;
}
