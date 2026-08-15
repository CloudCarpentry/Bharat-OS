---
title: Heterogeneous memory and tensor quick start
status: Draft
owner: Team
last_updated: '2024-01-01'
tags:
- doc
see_also:
- none
---
# Heterogeneous memory and tensor quick start

Link `Bharat::compute` (or `Bharat::sdk`) and include the public SDK headers:

```c
#include <bharat/compute/tensor.h>

bh_tensor_desc_t desc = {
    .dtype = BH_DTYPE_F32,
    .rank = 2,
    .shape = {64, 128},
    .layout = BH_TENSOR_LAYOUT_DENSE,
};
bh_tensor_t tensor;
bh_status_t st = bh_tensor_create(&desc, &tensor);
if (st != BH_OK) {
    /* handle error */
}

void *ptr = NULL;
st = bh_hmem_map_cpu(tensor.memory, BH_HMEM_ACCESS_WRITE, &ptr);
if (st == BH_OK) {
    float *values = ptr;
    values[0] = 1.0f;
    st = bh_hmem_unmap_cpu(tensor.memory);
}
if (st == BH_OK) {
    st = bh_hmem_sync_for_device(tensor.memory, 0, tensor.byte_length);
}
/* A future GPU/NPU/DPU queue consumes the same HMEM handle. */
(void)bh_tensor_destroy(&tensor);
```

The hosted SDK backend allocates aligned CPU memory for tests and development. Zero-on-allocation is always requested by `bh_tensor_create`. Sync validates ranges and supplies ordering. It does not claim accelerator support or manufacture an IOVA. Views are non-owning, so destroy them before the owning tensor and never use a view after its owner is destroyed.
