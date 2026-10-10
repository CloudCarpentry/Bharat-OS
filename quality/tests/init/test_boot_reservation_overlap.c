#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <boot/boot_info.h>
#include "mm/pmm/early_alloc.h"

uint8_t _end[1];
static _Alignas(4096) uint8_t arena[32768];
void console_write_raw(const char *s, size_t n) { (void)s; (void)n; }
size_t string_length(const char *s) { return strlen(s); }
void pmm_boot_reservations_init(const boot_info_t *boot);

int main(void) {
    boot_info_t boot = {0};
    boot.module_count = 2;
    boot.modules[0].phys_start = (uintptr_t)(arena + 4096);
    boot.modules[0].size = 4096;
    boot.modules[1].phys_start = (uintptr_t)(arena + 12288);
    boot.modules[1].size = 4096;
    memset(arena, 0xA5, sizeof(arena));
    pmm_boot_reservations_init(&boot);

    /* Endpoints are free; retry must skip both interior and later modules. */
    early_alloc_init((uintptr_t)arena);
    void *allocation = early_alloc(12288, 4096);
    for (unsigned i = 4096; i < 8192; ++i) assert(arena[i] == 0xA5);
    for (unsigned i = 12288; i < 16384; ++i) assert(arena[i] == 0xA5);

    assert(allocation == arena + 16384);

    /* Alignment itself can move a previously free bump pointer into a module. */
    early_alloc_init((uintptr_t)(arena + 4095));
    assert(early_alloc(32, 4096) == arena + 8192);
    early_alloc_init((uintptr_t)arena);
    assert(early_alloc(4096, 4096) == arena); /* exclusive end is safe */
    early_alloc_init(UINT64_MAX - 4);
    assert(early_alloc(8, 1) == NULL);
    assert(early_alloc(1, 16) == NULL);
    puts("PASS: interior reservations, alignment, boundaries and overflow");
}
