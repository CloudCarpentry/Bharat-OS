#include <stddef.h>
#include "socket_table.h"
#include "driver_virtio_adapter.h"

int main(void) {
    // Scaffold daemon phase 2

    // Initialize stack modules
    socket_table_init();
    virtio_adapter_init();

    while(1) {
        // Run polling loop for driver, timers, or ARP timeouts in future
    }

    return 0;
}
