#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include "bharat/mem_class.h"
#include "mm/mem_model.h"
#include "kernel/status.h"

/* Mock the memory model behavior for testing admission logic */
static mem_model_t mock_model = MEM_MODEL_MMU_FULL;

mem_model_t mem_model_get_current(void) {
    return mock_model;
}

/* Prototype of the function we are testing is now in mm/mem_model.h */
extern int sys_mem_alloc_class(size_t size, uint32_t mem_class, uint32_t flags, uint64_t* out_addr);

void test_mem_class_admission() {
    printf("Testing memory class admission logic...\n");

    /* Test Tier U on MMU_FULL */
    mock_model = MEM_MODEL_MMU_FULL;
    assert(mem_class_is_supported(MEM_TENSOR, mock_model) == true);
    assert(mem_class_is_supported(MEM_MODEL_RO, mock_model) == true);
    assert(mem_class_is_supported(MEM_SCRATCH_LOWLAT, mock_model) == true);

    /* Test Tier P on MMU_FULL */
    assert(mem_class_is_supported(MEM_TENSOR_PINNED, mock_model) == true);
    assert(mem_class_is_supported(MEM_SHARED_ACCEL, mock_model) == true);

    /* Test Tier U on MPU */
    mock_model = MEM_MODEL_MPU;
    assert(mem_class_is_supported(MEM_TENSOR, mock_model) == true);
    assert(mem_class_is_supported(MEM_MODEL_RO, mock_model) == true);
    assert(mem_class_is_supported(MEM_SCRATCH_LOWLAT, mock_model) == true);

    /* Test Tier P rejection on MPU */
    assert(mem_class_is_supported(MEM_TENSOR_PINNED, mock_model) == false);
    assert(mem_class_is_supported(MEM_SHARED_ACCEL, mock_model) == false);
    assert(mem_class_is_supported(MEM_STREAM_DMA, mock_model) == false);

    printf("Memory class admission logic tests passed!\n");
}

void test_sys_mem_alloc_class_enforcement() {
    printf("Testing sys_mem_alloc_class enforcement...\n");
    uint64_t out_addr;

    /* MMU_FULL allows everything */
    mock_model = MEM_MODEL_MMU_FULL;
    assert(sys_mem_alloc_class(4096, MEM_TENSOR, 0, &out_addr) == K_OK);
    assert(sys_mem_alloc_class(4096, MEM_TENSOR_PINNED, 0, &out_addr) == K_OK);

    /* MPU rejects Tier P */
    mock_model = MEM_MODEL_MPU;
    assert(sys_mem_alloc_class(4096, MEM_TENSOR, 0, &out_addr) == K_OK);
    assert(sys_mem_alloc_class(4096, MEM_TENSOR_PINNED, 0, &out_addr) == K_ERR_PROFILE_RESTRICTED);

    printf("sys_mem_alloc_class enforcement tests passed!\n");
}

int main() {
    test_mem_class_admission();
    test_sys_mem_alloc_class_enforcement();
    printf("All host tests for memory model AI-adjacent primitives passed!\n");
    return 0;
}
