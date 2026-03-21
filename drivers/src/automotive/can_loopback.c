#include "bharat/drivers/can.h"
#include <stddef.h>

static bool is_initialized = false;
#define CAN_LOOPBACK_MAX_FRAMES 16U
#define CAN_LOOPBACK_MAX_DLC 64U

typedef struct {
    uint32_t id;
    uint8_t dlc;
    uint8_t payload[CAN_LOOPBACK_MAX_DLC];
} can_loopback_frame_t;

static can_loopback_frame_t g_queue[CAN_LOOPBACK_MAX_FRAMES];
static uint8_t g_head;
static uint8_t g_tail;
static uint8_t g_count;
static uint32_t g_drop_count;

// Initialize the virtual CAN loopback driver
void can_loopback_init(void) {
    if (!is_initialized) {
        g_head = 0U;
        g_tail = 0U;
        g_count = 0U;
        g_drop_count = 0U;
        is_initialized = true;
    }
}

// Send a CAN frame through the loopback driver
bool can_loopback_send(uint32_t id, const uint8_t* data, uint8_t dlc) {
    if (!is_initialized || !data || dlc == 0U || dlc > CAN_LOOPBACK_MAX_DLC) {
        return false;
    }

    if (g_count >= CAN_LOOPBACK_MAX_FRAMES) {
        g_drop_count++;
        return false;
    }

    can_loopback_frame_t* slot = &g_queue[g_tail];
    slot->id = id;
    slot->dlc = dlc;
    for (uint8_t i = 0; i < dlc; ++i) {
        slot->payload[i] = data[i];
    }

    g_tail = (uint8_t)((g_tail + 1U) % CAN_LOOPBACK_MAX_FRAMES);
    g_count++;
    return true;
}

bool can_loopback_receive(uint32_t* id, uint8_t* data, uint8_t* dlc) {
    if (!is_initialized || g_count == 0U || !id || !data || !dlc) {
        return false;
    }

    can_loopback_frame_t* slot = &g_queue[g_head];
    *id = slot->id;
    *dlc = slot->dlc;
    for (uint8_t i = 0; i < slot->dlc; ++i) {
        data[i] = slot->payload[i];
    }

    g_head = (uint8_t)((g_head + 1U) % CAN_LOOPBACK_MAX_FRAMES);
    g_count--;
    return true;
}

uint8_t can_loopback_pending(void) {
    return g_count;
}

uint32_t can_loopback_drop_count(void) {
    return g_drop_count;
}

void can_loopback_reset(void) {
    g_head = 0U;
    g_tail = 0U;
    g_count = 0U;
    g_drop_count = 0U;
}
