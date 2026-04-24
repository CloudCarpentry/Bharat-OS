#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "urpc.h"

// Macro to assert conditions
#define ASSERT(cond) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "Assertion failed: %s, file %s, line %d\n", \
                    #cond, __FILE__, __LINE__); \
            exit(1); \
        } \
    } while (0)

int main() {
    urpc_channel_t channel;
    uint8_t buffer[1024]; // Large enough for our tests

    int ret;

    // Test 1: size_bytes < sizeof(urpc_msg_t) -> Returns invalid
    ret = urpc_init_channel(&channel, buffer, sizeof(urpc_msg_t) - 1);
    ASSERT(ret == URPC_ERR_INVALID);

    // Test 2: size_bytes == sizeof(urpc_msg_t) -> Capacity is 1
    ret = urpc_init_channel(&channel, buffer, sizeof(urpc_msg_t));
    ASSERT(ret == URPC_SUCCESS);
    ASSERT(channel.capacity == 1);

    // Test 3: size_bytes == 3 * sizeof(urpc_msg_t) -> Capacity rounded down to 2
    ret = urpc_init_channel(&channel, buffer, 3 * sizeof(urpc_msg_t));
    ASSERT(ret == URPC_SUCCESS);
    ASSERT(channel.capacity == 2);

    // Test 4: size_bytes == 4 * sizeof(urpc_msg_t) -> Capacity stays 4
    ret = urpc_init_channel(&channel, buffer, 4 * sizeof(urpc_msg_t));
    ASSERT(ret == URPC_SUCCESS);
    ASSERT(channel.capacity == 4);

    // Test 5: size_bytes == 7 * sizeof(urpc_msg_t) -> Capacity rounded down to 4
    ret = urpc_init_channel(&channel, buffer, 7 * sizeof(urpc_msg_t));
    ASSERT(ret == URPC_SUCCESS);
    ASSERT(channel.capacity == 4);

    // Additional Test: Null pointers
    ret = urpc_init_channel(NULL, buffer, sizeof(urpc_msg_t));
    ASSERT(ret == URPC_ERR_INVALID);

    ret = urpc_init_channel(&channel, NULL, sizeof(urpc_msg_t));
    ASSERT(ret == URPC_ERR_INVALID);

    printf("All urpc_init_channel tests passed.\n");
    return 0;
}
