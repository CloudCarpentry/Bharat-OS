#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "bharat/msg/transport.h"

void test_loopback_transport() {
    bharat_transport_t transport;
    assert(bharat_transport_loopback_create(&transport, 1024) == BHARAT_MSG_OK);

    // Empty poll
    assert(transport.ops->poll(&transport, 0) == 0);

    const char* payload = "Hello Transport";
    size_t payload_len = strlen(payload) + 1;

    // Send
    assert(transport.ops->send(&transport, (const uint8_t*)payload, payload_len) == BHARAT_MSG_OK);

    // Should be ready now
    assert(transport.ops->poll(&transport, 0) == 1);

    // Receive
    uint8_t rx_buf[1024];
    size_t rx_len = 0;
    assert(transport.ops->recv(&transport, rx_buf, sizeof(rx_buf), &rx_len) == BHARAT_MSG_OK);
    assert(rx_len == payload_len);
    assert(strcmp((const char*)rx_buf, payload) == 0);

    // Verify empty state
    assert(transport.ops->poll(&transport, 0) == 0);

    assert(transport.ops->close(&transport) == BHARAT_MSG_OK);
}

int main() {
    printf("Running Transport Loopback tests...\n");
    test_loopback_transport();
    printf("All Transport Loopback tests passed!\n");
    return 0;
}
