#include <stddef.h>
#include "socket_table.h"
#include "driver_virtio_adapter.h"
#include "bharat/component_version.h"
#include "bharat/buildinfo.h"

BHARAT_REGISTER_COMPONENT(
    BHARAT_COMPONENT_NAME,
    BHARAT_COMPONENT_KIND,
    BHARAT_COMPONENT_VERSION,
    BHARAT_COMPONENT_IFACE,
    0, /* abi version */
    BHARAT_COMPONENT_CHANNEL,
    BHARAT_GIT_SHA,
    BHARAT_GIT_DIRTY,
    BHARAT_BUILD_EPOCH,
    BHARAT_BUILD_TIME_UTC
);

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
