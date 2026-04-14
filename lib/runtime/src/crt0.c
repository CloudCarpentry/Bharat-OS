#include <stdint.h>

extern int main(int argc, char** argv);
extern void bharat_exit(int status);

__attribute__((used))
void _start(void) {
    // For now, simple _start for freestanding C
    int ret = main(0, 0);
    bharat_exit(ret);
    while (1) {}
}
