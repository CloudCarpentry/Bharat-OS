#include "../../include/mm/accel_mem.h"
#include "../../include/kernel/status.h"
#include "../../include/mm/pmm.h"
#include "../../include/mm/dma.h"
#include "../../include/hal/hal_dma.h"
#include <stddef.h>

extern void *memset(void *s, int c, size_t n);

static size_t g_accel_pinned_bytes = 0;
#define ACCEL_MAX_PINNED_BUDGET (1024ULL * 1024ULL * 512ULL) // 512MB for now

int accel_mem_buffer_create(size_t size, size_t alignment, accel_mem_flags_t flags, cap_handle_t owner, accel_buffer_t **out_buf) {
    if (!out_buf || size == 0) return K_ERR_INVALID_ARG;

    extern void* kmalloc(size_t);
    extern void kfree(void*);

    accel_buffer_t *buf = kmalloc(sizeof(accel_buffer_t));
    if (!buf) return K_ERR_NO_MEMORY;

    memset(buf, 0, sizeof(accel_buffer_t));
    buf->size = size;
    buf->alignment = alignment;
    buf->flags = flags;
    buf->owner_cap = owner;
    buf->state = ACCEL_BUF_STATE_CREATED;
    buf->ref_count = 1;

    buf->num_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    buf->phys_pages = kmalloc(buf->num_pages * sizeof(uintptr_t));
    if (!buf->phys_pages) {
        kfree(buf);
        return K_ERR_NO_MEMORY;
    }

    *out_buf = buf;
    return K_OK;
}

void accel_mem_buffer_destroy(accel_buffer_t *buf) {
    if (!buf) return;
    accel_mem_teardown(buf);

    extern void kfree(void*);
    if (buf->phys_pages) {
        kfree(buf->phys_pages);
    }
    kfree(buf);
}

int accel_mem_pin(accel_buffer_t *buf) {
    if (!buf) return K_ERR_INVALID_ARG;
    if (buf->state >= ACCEL_BUF_STATE_PINNED && buf->pin_count > 0) {
        buf->pin_count++;
        return K_OK;
    }

    if (g_accel_pinned_bytes + buf->size > ACCEL_MAX_PINNED_BUDGET) {
        return K_ERR_PMM_EXHAUSTED;
    }

    for (size_t i = 0; i < buf->num_pages; i++) {
        void *page = pmm_alloc_page(0); // ZONE_NORMAL
        if (!page) {
            return K_ERR_NO_MEMORY;
        }
        // Minimal baseline abstraction
        buf->phys_pages[i] = (uintptr_t)page;
    }

    g_accel_pinned_bytes += buf->size;
    buf->pin_count = 1;
    buf->state = ACCEL_BUF_STATE_PINNED;

    return K_OK;
}

int accel_mem_unpin(accel_buffer_t *buf) {
    if (!buf || buf->pin_count == 0) return K_ERR_INVALID_ARG;

    if (buf->state > ACCEL_BUF_STATE_PINNED) {
        return K_ERR_BUSY;
    }

    buf->pin_count--;
    if (buf->pin_count == 0) {
        for (size_t i = 0; i < buf->num_pages; i++) {
            // pmm_free_page((void*)buf->phys_pages[i]);
            buf->phys_pages[i] = 0;
        }
        g_accel_pinned_bytes -= buf->size;
        buf->state = ACCEL_BUF_STATE_CREATED;
    }

    return K_OK;
}

int accel_mem_sync(accel_buffer_t *buf, accel_sync_dir_t dir) {
    if (!buf) return K_ERR_INVALID_ARG;

    if (buf->flags & ACCEL_MEM_COHERENT) {
        if (hal_dma_is_coherent()) {
            return K_OK;
        }
    }

    if ((buf->flags & ACCEL_MEM_STREAMING) || !hal_dma_is_coherent()) {
        hal_dma_buffer_t dma_buf = {0};

        if (buf->state >= ACCEL_BUF_STATE_SG_BUILT && buf->sg_table) {
            accel_sg_entry_t *entry = buf->sg_table->head;
            while(entry) {
                dma_buf.phys_addr = entry->phys_addr;
                dma_buf.size = entry->length;
                if (dir == ACCEL_SYNC_TO_DEVICE || dir == ACCEL_SYNC_BIDIRECTIONAL) {
                    hal_dma_sync_for_device(&dma_buf);
                }
                if (dir == ACCEL_SYNC_FROM_DEVICE || dir == ACCEL_SYNC_BIDIRECTIONAL) {
                    hal_dma_sync_for_cpu(&dma_buf);
                }
                entry = entry->next;
            }
        }
    }

    return K_OK;
}
