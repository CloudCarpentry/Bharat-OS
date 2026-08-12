#ifndef BHARAT_SDK_COMPUTE_TENSOR_H
#define BHARAT_SDK_COMPUTE_TENSOR_H

#include <bharat/hmem.h>

#define BH_TENSOR_MAX_RANK 8U
#define BH_TENSOR_F_OWNS_MEMORY (UINT32_C(1) << 0)
#define BH_TENSOR_F_VIEW (UINT32_C(1) << 1)

typedef enum bh_dtype {
  BH_DTYPE_INVALID = 0,
  BH_DTYPE_F32,
  BH_DTYPE_F16,
  BH_DTYPE_BF16,
  BH_DTYPE_I8,
  BH_DTYPE_U8,
  BH_DTYPE_I16,
  BH_DTYPE_I32,
  BH_DTYPE_BOOL
} bh_dtype_t;
typedef enum bh_tensor_layout {
  BH_TENSOR_LAYOUT_DENSE = 0,
  BH_TENSOR_LAYOUT_STRIDED = 1
} bh_tensor_layout_t;

typedef struct bh_tensor {
  bharat_hmem_handle_t memory;
  uint32_t rank;
  uint32_t dtype;
  uint64_t shape[BH_TENSOR_MAX_RANK];
  uint64_t stride[BH_TENSOR_MAX_RANK];
  uint64_t byte_offset;
  uint64_t byte_length;
  uint32_t layout;
  uint32_t flags;
} bh_tensor_t;

typedef struct bh_tensor_desc {
  uint32_t dtype;
  uint32_t rank;
  uint64_t shape[BH_TENSOR_MAX_RANK];
  uint64_t stride[BH_TENSOR_MAX_RANK];
  uint32_t layout;
  uint32_t reserved;
  uint64_t hmem_property_flags;
} bh_tensor_desc_t;

typedef struct bh_tensor_view_desc {
  uint32_t rank;
  uint32_t layout;
  uint64_t shape[BH_TENSOR_MAX_RANK];
  uint64_t stride[BH_TENSOR_MAX_RANK];
  uint64_t byte_offset;
} bh_tensor_view_desc_t;

bh_status_t bh_tensor_validate(const bh_tensor_t *tensor);
bh_status_t bh_tensor_create(const bh_tensor_desc_t *desc, bh_tensor_t *out);
bh_status_t bh_tensor_destroy(bh_tensor_t *tensor);
bh_status_t bh_tensor_view(const bh_tensor_t *parent,
                           const bh_tensor_view_desc_t *view, bh_tensor_t *out);

#endif
