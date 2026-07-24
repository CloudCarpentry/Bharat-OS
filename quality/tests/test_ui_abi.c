#include <assert.h>
#include <stddef.h>
#include "bharat/uapi/display/display.h"
#include "bharat/uapi/display/buffer.h"

int main(void) {
    /* Verify display mode layout */
    assert(sizeof(bharat_display_mode_t) >= 20);

    /* Verify buffer descriptor layout */
    assert(sizeof(bharat_display_buffer_t) >= 32);

    /* Verify lifecycle enums */
    assert(BHARAT_BUFFER_STATE_ALLOCATED == 0);
    assert(BHARAT_BUFFER_STATE_RELEASED == 4);

    /* Verify display class enums */
    assert(BHARAT_DISPLAY_CLASS_HEADLESS == 5);

    return 0;
}
