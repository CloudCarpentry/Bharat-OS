#include <stdio.h>
#include <stdint.h>

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    // TODO: Map device registers into this process space using kernel APIs
    // TODO: Await interrupts via IPC from the kernel
    // TODO: Map URPC shared memory for high-bandwidth endpoints (Network, Disk)
    return 0;
}
