#include <stdio.h>
#include <assert.h>
// Inclusion test to ensure no redefinition conflicts
#include <stddef.h>
#include <stdint.h>
#include "../lib/posix/include/unistd.h"

int main() {
    printf("Running POSIX header integration tests...\n");

    // Check size of newly added types to ensure they are available
    assert(sizeof(uid_t) == 4);
    assert(sizeof(gid_t) == 4);
    assert(sizeof(pid_t) == 4);

    printf("POSIX header integration tests passed successfully.\n");
    return 0;
}
