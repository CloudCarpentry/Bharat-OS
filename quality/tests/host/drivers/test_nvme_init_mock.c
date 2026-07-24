#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../../../core/drivers/storage/nvme/nvme_core.h"
#include "../../../core/drivers/storage/nvme/nvme_regs.h"
#include "../../../core/drivers/storage/nvme/nvme_mock.h"

// Define a small buffer to represent MMIO
static __attribute__((aligned(8))) uint8_t mock_mmio[4096];

void mock_write32(volatile uint8_t *base, uint32_t offset, uint32_t value) {
    *(uint32_t *)(base + offset) = value;
}

void mock_write64(volatile uint8_t *base, uint32_t offset, uint64_t value) {
    *(uint64_t *)(base + offset) = value;
}

int main(void) {
    printf("Running NVMe init mock tests...\n");

    nvme_controller_t ctrl;
    memset(&ctrl, 0, sizeof(ctrl));
    memset(mock_mmio, 0, sizeof(mock_mmio));

    // Test invalid MMIO pointers
    int ret = nvme_controller_init(NULL, mock_mmio);
    assert(ret != 0);
    ret = nvme_controller_init(&ctrl, NULL);
    assert(ret != 0);

    // Test failing condition: All zeroes vs/cap
    ret = nvme_controller_init(&ctrl, mock_mmio);
    assert(ret != 0);
    printf("Handled empty/zero MMIO correctly.\n");

    // Populate mock MMIO with valid initial state
    mock_write32(mock_mmio, NVME_REG_VS, 0x00010200); // v1.2
    mock_write64(mock_mmio, NVME_REG_CAP, 0x000000000000FFFF); // valid cap

    // Simulate controller initially disabled, ready state 0
    mock_write32(mock_mmio, NVME_REG_CC, 0);
    mock_write32(mock_mmio, NVME_REG_CSTS, 0);

    // Provide a mocked hardware response to the enable bit:
    // When CC_EN is written, CSTS_RDY should follow.
    // In a real mock we would use a thread or hook, but our scaffold
    // just checks bits. Let's prime the CSTS with RDY to skip the wait loops.
    // Wait, the code waits for it to become ready if it sets EN.
    // Since we are single-threaded and the test doesn't actually intercept writes,
    // we'll modify the `CSTS` right here to be `RDY` so the loop exits immediately.
    // But wait, the loop checks for the bit. Since we can't change it asynchronously,
    // the loop will timeout. Let's patch `CSTS` to always report what it needs
    // *if* it's spinning. Actually, since the scaffold has `retries--` it'll just timeout.

    // If it times out, `nvme_controller_init` returns -1.
    // To make it pass, we can just set CSTS_RDY *before* calling init.
    // But init checks:
    // while (!(mmio_read32(mmio_base, NVME_REG_CSTS) & NVME_CSTS_RDY) && retries-- > 0)
    // If we set it beforehand, the loop completes instantly.
    mock_write32(mock_mmio, NVME_REG_CSTS, NVME_CSTS_RDY);

    ret = nvme_controller_init(&ctrl, mock_mmio);
    assert(ret == 0);
    printf("NVMe mock init succeeded with valid MMIO bits.\n");

    // Test Admin Queue Mock Submit
    ret = nvme_submit_mock_cmd(&ctrl, 0, 0x01); // Admin command
    assert(ret == 0);
    assert(ctrl.admin_queue.sq_tail == 1);
    printf("Mock NVMe admin command submission succeeded.\n");

    // Test Identify
    uint8_t id_buffer[4096];
    ret = nvme_admin_identify(&ctrl, id_buffer, sizeof(id_buffer));
    assert(ret == 0);
    assert(strncmp((char *)id_buffer + 24, "MOCK_NVME_CONTROLLER", 20) == 0);
    printf("Mock NVMe admin identify succeeded.\n");

    // Test Shutdown
    nvme_controller_shutdown(&ctrl);
    assert(ctrl.state == 0); // NVME_STATE_UNINIT
    printf("NVMe mock shutdown succeeded.\n");

    printf("All NVMe tests passed!\n");
    return 0;
}