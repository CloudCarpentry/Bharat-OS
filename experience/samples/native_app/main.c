#include <stdio.h>
#include <stdlib.h>
#include <bharat/bh_native.h>

int main(int argc, char* argv[]) {
    printf("Welcome to Bharat-OS Native App!\n");

    /* Test malloc */
    void* ptr = malloc(1024);
    if (ptr) {
        printf("Malloc success: %p\n", ptr);
        free(ptr);
    }

    /* Test memory classes */
    void* rt_mem = NULL;
    int ret = bh_alloc_ex(4096, BH_MEM_CLASS_RT, 0, &rt_mem);
    if (ret == 0) {
        printf("RT memory allocated at: %p\n", rt_mem);
    }

    /* Test time */
    bh_time_t now;
    bh_time_get(BH_CLOCK_MONOTONIC, &now);
    printf("Current monotonic time: %llu\n", (unsigned long long)now);

    return 0;
}
