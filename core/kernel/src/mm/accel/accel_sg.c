#include "../../include/mm/accel_mem.h"
#include "../../include/kernel/status.h"
#include "../../include/mm/pmm.h"
#include <stddef.h>

int accel_sg_build(accel_buffer_t *buf) {
    if (!buf || buf->state < ACCEL_BUF_STATE_PINNED) return K_ERR_INVALID_ARG;
    if (buf->state >= ACCEL_BUF_STATE_SG_BUILT) return K_ERR_ALREADY_EXISTS;

    extern void* kmalloc(size_t);
    accel_sg_table_t *sgt = kmalloc(sizeof(accel_sg_table_t));
    if (!sgt) return K_ERR_NO_MEMORY;

    sgt->head = NULL;
    sgt->tail = NULL;
    sgt->num_entries = 0;
    sgt->total_length = 0;

    accel_sg_entry_t *current = NULL;

    for (size_t i = 0; i < buf->num_pages; i++) {
        uintptr_t paddr = buf->phys_pages[i];

        if (current && (current->phys_addr + current->length) == paddr) {
            current->length += PAGE_SIZE;
        } else {
            accel_sg_entry_t *entry = kmalloc(sizeof(accel_sg_entry_t));
            if (!entry) {
                accel_sg_entry_t *node = sgt->head;
                while (node) {
                    accel_sg_entry_t *next = node->next;
                    extern void kfree(void*);
                    kfree(node);
                    node = next;
                }
                extern void kfree(void*);
                kfree(sgt);
                return K_ERR_NO_MEMORY;
            }

            entry->phys_addr = paddr;
            entry->length = PAGE_SIZE;
            entry->offset = 0;
            entry->next = NULL;

            if (!sgt->head) sgt->head = entry;
            if (sgt->tail) sgt->tail->next = entry;
            sgt->tail = entry;
            sgt->num_entries++;
            current = entry;
        }
        sgt->total_length += PAGE_SIZE;
    }

    buf->sg_table = sgt;
    buf->state = ACCEL_BUF_STATE_SG_BUILT;
    return K_OK;
}

void accel_sg_release(accel_buffer_t *buf) {
    if (!buf || buf->state < ACCEL_BUF_STATE_SG_BUILT) return;

    if (buf->state > ACCEL_BUF_STATE_SG_BUILT) {
        return;
    }

    if (buf->sg_table) {
        accel_sg_entry_t *node = buf->sg_table->head;
        while (node) {
            accel_sg_entry_t *next = node->next;
            extern void kfree(void*);
            kfree(node);
            node = next;
        }
        extern void kfree(void*);
        kfree(buf->sg_table);
        buf->sg_table = NULL;
    }

    buf->state = ACCEL_BUF_STATE_PINNED;
}
