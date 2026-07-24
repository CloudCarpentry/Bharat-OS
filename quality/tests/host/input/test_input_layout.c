#include <assert.h>
#include <stdio.h>
#include "bharat/uapi/input/input_event.h"

int main(void) {
    printf("Running test_input_layout...\n");

    /* Verify event size and layout */
    assert(sizeof(bharat_input_event_t) == 24);

    bharat_input_event_t ev = {
        .timestamp_ns = 1000,
        .type = BHARAT_INPUT_KEY,
        .code = 30,
        .value = 1,
        .device_id = 5
    };

    assert(ev.timestamp_ns == 1000);
    assert(ev.type == 1);
    assert(ev.device_id == 5);

    printf("test_input_layout passed!\n");
    return 0;
}
