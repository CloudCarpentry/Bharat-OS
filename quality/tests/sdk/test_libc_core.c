#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

void test_string() {
    char buf[64];
    memset(buf, 0, sizeof(buf));
    strcpy(buf, "Bharat");
    assert(strcmp(buf, "Bharat") == 0);
    assert(strlen(buf) == 6);
    printf("test_string passed\n");
}

void test_malloc() {
    void* p1 = malloc(10);
    void* p2 = malloc(20);
    assert(p1 != NULL);
    assert(p2 != NULL);
    assert(p1 != p2);
    free(p1);
    free(p2);
    printf("test_malloc passed\n");
}

int main() {
    test_string();
    test_malloc();
    printf("All SDK host tests passed!\n");
    return 0;
}
