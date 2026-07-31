#include "irq/bh_irq.h"
#include "spinlock.h"
#include "kernel/status.h"
#include "kernel_safety.h"

#define VECTOR_MIN 0x30U
#define VECTOR_MAX 0xDFU
#define VECTOR_BITMAP_WORDS 8U

static uint32_t g_vector_bitmap[VECTOR_BITMAP_WORDS];
static spinlock_t g_vector_lock;
static bool g_vector_lock_initialized = false;

static void ensure_vector_lock_init(void) {
    if (!g_vector_lock_initialized) {
        spin_lock_init(&g_vector_lock);
        g_vector_lock_initialized = true;
        for (uint32_t i = 0; i < VECTOR_BITMAP_WORDS; i++) {
            g_vector_bitmap[i] = 0;
        }
    }
}

kstatus_t bh_irq_vector_alloc(uint32_t count, uint32_t alignment, uint32_t flags, bh_irq_vector_set_t *out) {
    (void)flags;
    if (count == 0 || !out) return K_ERR_INVALID_ARG;
    if (alignment == 0) alignment = 1;

    ensure_vector_lock_init();
    spin_lock(&g_vector_lock);

    // Find a contiguous block of 'count' vectors aligned to 'alignment'
    uint32_t found_start = 0;
    bool found = false;

    for (uint32_t candidate = VECTOR_MIN; candidate + count - 1 <= VECTOR_MAX; candidate++) {
        if (candidate % alignment != 0) continue;

        bool range_free = true;
        for (uint32_t i = 0; i < count; i++) {
            uint32_t vec = candidate + i;
            uint32_t word = vec / 32U;
            uint32_t bit = vec % 32U;
            if (word < VECTOR_BITMAP_WORDS && (g_vector_bitmap[word] & (1U << bit))) {
                range_free = false;
                break;
            }
        }

        if (range_free) {
            found_start = candidate;
            found = true;
            break;
        }
    }

    if (!found) {
        spin_unlock(&g_vector_lock);
        return K_ERR_NO_MEMORY; // Fail-closed exhaustion
    }

    // Allocate the range (set bits to 1)
    for (uint32_t i = 0; i < count; i++) {
        uint32_t vec = found_start + i;
        uint32_t word = vec / 32U;
        uint32_t bit = vec % 32U;
        g_vector_bitmap[word] |= (1U << bit);
    }

    out->start = found_start;
    out->count = count;

    spin_unlock(&g_vector_lock);
    return K_OK;
}

void bh_irq_vector_free(const bh_irq_vector_set_t *set) {
    if (!set || set->count == 0) return;

    ensure_vector_lock_init();
    spin_lock(&g_vector_lock);

    // Verify all vectors in the set are currently allocated, to prevent double-free
    for (uint32_t i = 0; i < set->count; i++) {
        uint32_t vec = set->start + i;
        if (vec < VECTOR_MIN || vec > VECTOR_MAX) {
            // Reserved-vector assertions: attempting to free out-of-bounds or reserved vectors
            spin_unlock(&g_vector_lock);
            return;
        }
        uint32_t word = vec / 32U;
        uint32_t bit = vec % 32U;
        if (!(g_vector_bitmap[word] & (1U << bit))) {
            // Double-free or unallocated free detection
            spin_unlock(&g_vector_lock);
            return;
        }
    }

    // Safe to free
    for (uint32_t i = 0; i < set->count; i++) {
        uint32_t vec = set->start + i;
        uint32_t word = vec / 32U;
        uint32_t bit = vec % 32U;
        g_vector_bitmap[word] &= ~(1U << bit);
    }

    spin_unlock(&g_vector_lock);
}
