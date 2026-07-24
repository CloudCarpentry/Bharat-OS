#include "nvme_core.h"
#include <string.h>

int nvme_admin_identify(nvme_controller_t *ctrl, void *buffer, size_t len) {
    if (!ctrl || ctrl->state != 1 || !buffer || len < 4096) return -1;

    // Simulate writing identify data
    memset(buffer, 0, len);
    strncpy((char *)buffer + 24, "MOCK_NVME_CONTROLLER_1.0", 32);

    return 0; // Success mock
}

int nvme_submit_mock_cmd(nvme_controller_t *ctrl, uint32_t qid, uint8_t opcode) {
    if (!ctrl || ctrl->state != 1) return -1;
    if (qid != 0) return -1; // Only admin queue supported in mock

    // Update admin queue pointers (mocking submission)
    ctrl->admin_queue.sq_tail = (ctrl->admin_queue.sq_tail + 1) % ctrl->admin_queue.size;
    ctrl->admin_queue.cq_head = (ctrl->admin_queue.cq_head + 1) % ctrl->admin_queue.size;

    return 0; // Success
}