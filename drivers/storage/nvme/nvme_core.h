#ifndef BHARATOS_NVME_CORE_H
#define BHARATOS_NVME_CORE_H

#include <stdint.h>
#include <stddef.h>

struct nvme_controller;

typedef struct nvme_queue {
    uint32_t id;
    uint32_t size;
    void *sq_vaddr;
    uint64_t sq_paddr;
    void *cq_vaddr;
    uint64_t cq_paddr;
    uint32_t sq_tail;
    uint32_t cq_head;
    uint16_t phase;
} nvme_queue_t;

typedef struct nvme_controller {
    int id;
    volatile uint8_t *mmio_base;
    nvme_queue_t admin_queue;
    char name[32];
    int state;
} nvme_controller_t;

int nvme_controller_init(nvme_controller_t *ctrl, volatile uint8_t *mmio_base);
void nvme_controller_shutdown(nvme_controller_t *ctrl);

int nvme_admin_identify(nvme_controller_t *ctrl, void *buffer, size_t len);
int nvme_submit_mock_cmd(nvme_controller_t *ctrl, uint32_t qid, uint8_t opcode);

#endif /* BHARATOS_NVME_CORE_H */