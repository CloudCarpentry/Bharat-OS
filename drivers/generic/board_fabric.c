#include "bharat/drivers/generic_driver.h"

static bharat_generic_driver_status_t g_status = BHARAT_GENERIC_DRIVER_STATUS_UNINITIALIZED;

static int board_fabric_init(void) {
    g_status = BHARAT_GENERIC_DRIVER_STATUS_READY;
    return 0;
}

static int board_fabric_start(void) {
    if (g_status != BHARAT_GENERIC_DRIVER_STATUS_READY) {
        g_status = BHARAT_GENERIC_DRIVER_STATUS_FAILED;
        return -1;
    }

    return 0;
}

static bharat_generic_driver_status_t board_fabric_health(void) {
    return g_status;
}

const bharat_generic_driver_ops_t bharat_board_fabric_driver = {
    .name = "generic-board-fabric",
    .class_id = BHARAT_GENERIC_DRIVER_CLASS_BOARD,
    .init = board_fabric_init,
    .start = board_fabric_start,
    .health = board_fabric_health,
};
