#include <stdio.h>
#include <stdlib.h>
#include <bharat/uapi/syscall_nr.h>

#define CHECK_SYSCALL(name, expected_number) \
    do { \
        if (name != expected_number) { \
            fprintf(stderr, "ABI BREAK: Syscall %s changed from %d to %d\n", #name, expected_number, name); \
            exit(1); \
        } \
    } while(0)

int main(void) {
    printf("Testing Syscall ABI consistency...\n");

    /*
     * 1. Exact numeric value check (append-only/freeze rule).
     * Any existing assigned syscall number MUST NEVER change.
     */
    CHECK_SYSCALL(SYSCALL_NOP, 0);
    CHECK_SYSCALL(SYSCALL_THREAD_CREATE, 1);
    CHECK_SYSCALL(SYSCALL_THREAD_DESTROY, 2);
    CHECK_SYSCALL(SYSCALL_SCHED_YIELD, 3);
    CHECK_SYSCALL(SYSCALL_VMM_MAP_PAGE, 4);
    CHECK_SYSCALL(SYSCALL_VMM_UNMAP_PAGE, 5);
    CHECK_SYSCALL(SYSCALL_CAPABILITY_INVOKE, 6);
    CHECK_SYSCALL(SYSCALL_ENDPOINT_CREATE, 7);
    CHECK_SYSCALL(SYSCALL_ENDPOINT_SEND, 8);
    CHECK_SYSCALL(SYSCALL_ENDPOINT_RECEIVE, 9);
    CHECK_SYSCALL(SYSCALL_CAPABILITY_DELEGATE, 10);
    CHECK_SYSCALL(SYSCALL_SCHED_SLEEP, 11);
    CHECK_SYSCALL(SYSCALL_SCHED_SET_PRIORITY, 12);
    CHECK_SYSCALL(SYSCALL_SCHED_SET_AFFINITY, 13);

    /*
     * 2. Duplicate value detector.
     * Ensure no two syscalls map to the same number.
     * We do this by creating an array tracking used numbers.
     */
    int used_numbers[65536] = {0};

#define SYSCALL_DEF(name, number) \
    if (number < 0 || number >= 65536) { \
        fprintf(stderr, "ABI ERROR: Syscall %s has out-of-bounds number %d\n", #name, number); \
        exit(1); \
    } \
    if (used_numbers[number] != 0) { \
        fprintf(stderr, "ABI BREAK: Duplicate syscall number %d for %s\n", number, #name); \
        exit(1); \
    } \
    used_numbers[number] = 1;

#include <bharat/uapi/syscall_table.def>
#undef SYSCALL_DEF

    printf("Syscall ABI consistency tests passed.\n");
    return 0;
}
