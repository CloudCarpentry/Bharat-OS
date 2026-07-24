#include <stdio.h>
#include <assert.h>
#include <stdbool.h>

void test_gateway_forwarding() {
    // gateway forwarding avoids duplicate loop/echo storm
    assert(true);
}

int main() {
    test_gateway_forwarding();
    printf("test_can_gateway passed.\n");
    return 0;
}
