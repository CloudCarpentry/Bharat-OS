#include <assert.h>
#include <stdint.h>
#include <stdio.h>

void default_timer_isr(void) {
    // mock
}

#include "../kernel/include/trap.h"

static address_space_t g_as = { .root_table = 0x1000U };

address_space_t* mm_create_address_space(void) {
    return &g_as;
}

phys_addr_t mm_alloc_page(uint32_t preferred_numa_node) {
    (void)preferred_numa_node;
    return 0;
}

void mm_free_page(phys_addr_t page) {
    (void)page;
}





int vmm_map_page(virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    (void)vaddr;
    (void)paddr;
    (void)flags;
    return 0;
}

int vmm_unmap_page(virt_addr_t vaddr) {
    (void)vaddr;
    return 0;
}

void kernel_panic(const char* msg) {
    (void)msg;
    // Mock kernel panic
}

void user_entry(void) {}

int main(void) {
    sched_init();
    assert(trap_init() == 0);

    uint64_t tid = 0;
    long rc = syscall_dispatch(SYSCALL_THREAD_CREATE,
                               (uint64_t)(uintptr_t)user_entry,
                               (uint64_t)(uintptr_t)&tid,
                               0,0,0,0);
    assert(rc == 0);
    assert(tid != 0);

    assert(syscall_dispatch(SYSCALL_THREAD_DESTROY, tid, 0,0,0,0,0) == 0);

    // Invalid pointer should be rejected.
    assert(syscall_dispatch(SYSCALL_THREAD_CREATE,
                            (uint64_t)(uintptr_t)user_entry,
                            0x10U,
                            0,0,0,0) == -22);

    uint32_t send_cap = 0;
    uint32_t recv_cap = 0;
    assert(syscall_dispatch(SYSCALL_ENDPOINT_CREATE,
                            (uint64_t)(uintptr_t)&send_cap,
                            (uint64_t)(uintptr_t)&recv_cap,
                            0,0,0,0) == 0);

    const char payload[] = {'o','k'};
    assert(syscall_dispatch(SYSCALL_ENDPOINT_SEND,
                            send_cap,
                            (uint64_t)(uintptr_t)payload,
                            2U,0,0,0) == 0);

    uint8_t recv_buf[8] = {0};
    uint32_t recv_len = 0;
    assert(syscall_dispatch(SYSCALL_ENDPOINT_RECEIVE,
                            recv_cap,
                            (uint64_t)(uintptr_t)recv_buf,
                            sizeof(recv_buf),
                            (uint64_t)(uintptr_t)&recv_len,
                            0,0) == 0);
    assert(recv_len == 2U);
    assert(recv_buf[0] == 'o' && recv_buf[1] == 'k');

    trap_frame_t frame = {0};
    frame.cause =
#if defined(__x86_64__)
      0x80U;
#elif defined(__riscv)
      8U;
#else
      0xFFFFU;
#endif
    frame.from_user = 1U;
    frame.gpr[0] = SYSCALL_NOP;

    assert(trap_handle(&frame) == 0);
    assert(frame.gpr[0] == 0U);

    frame.from_user = 0U;
    frame.gpr[0] = SYSCALL_NOP;
    assert(trap_handle(&frame) == -1);

    printf("Trap/syscall gate tests passed.\n");
    return 0;
}





#include "../kernel/include/slab.h"
#include <stdlib.h>
#include <string.h>

// Some tests were crashing on free() because thread_destroy called kcache_free on pointers that weren't allocated by kcache_alloc (like stack/global variables in tests).
// Since these are stub tests without full memory management setup, let's just make kcache_free a no-op for tests.
kcache_t* kcache_create(const char* name, size_t size) {
    kcache_t* c = malloc(sizeof(kcache_t));
    if(c) {
        c->object_size = size;
        c->name = name;
    }
    return c;
}
void* kcache_alloc(kcache_t* cache) {
    if(!cache) return NULL;
    return malloc(cache->object_size);
}
void kcache_free(kcache_t* cache, void* obj) {
    // DO NOTHING in tests to avoid free() errors on statically allocated mock threads.
}

uint32_t hal_cpu_get_id(void) { return 0; }
void hal_cpu_halt(void) { }

void default_timer_isr(void) {
    /* Mock implementation for testing trap.c generic timer tick handling */
}
