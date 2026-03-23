#include <stdint.h>
#include <stdbool.h>

void f(volatile uint64_t* ptr) {
    __atomic_fetch_add(ptr, 1, __ATOMIC_SEQ_CST);
}
int main() {}
