#include <stdio.h>
#include <stdlib.h>
void panic(const char *msg) {
    printf("PANIC: %s\n", msg);
    abort();
}
