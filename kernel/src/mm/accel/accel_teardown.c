#include "../../include/mm/accel_mem.h"
#include "../../include/kernel/status.h"
#include "../../include/mm/pmm.h"
#include "../../include/mm/dma.h"
#include <stddef.h>

void accel_mem_teardown(accel_buffer_t *buf) {
    if (!buf) return;

    if (buf->state == ACCEL_BUF_STATE_MAPPED) {
        buf->state = ACCEL_BUF_STATE_IOVA_BOUND;
    }

    if (buf->state == ACCEL_BUF_STATE_IOVA_BOUND) {
        if (buf->iommu_domain) {
            accel_iova_unbind(buf);
        }
    }

    if (buf->state == ACCEL_BUF_STATE_SG_BUILT) {
        accel_sg_release(buf);
    }

    if (buf->state == ACCEL_BUF_STATE_PINNED) {
        while (buf->pin_count > 0) {
            accel_mem_unpin(buf);
        }
    }

    if (buf->state == ACCEL_BUF_STATE_CREATED) {
        buf->state = ACCEL_BUF_STATE_DESTROYED;
    }
}
