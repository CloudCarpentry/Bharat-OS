#include <assert.h>
#include <bharat/compute/tensor.h>
#include <stdint.h>
#include <string.h>

static void hmem_lifecycle(void) {
  bharat_hmem_desc_v1_t d = {
      .version = 1,
      .struct_size = sizeof(d),
      .size = 128,
      .alignment = 64,
      .usage_flags = BH_HMEM_USAGE_CPU_READ | BH_HMEM_USAGE_CPU_WRITE,
      .property_flags = BH_HMEM_PROP_ZERO_ON_ALLOC | BH_HMEM_PROP_CPU_COHERENT,
      .preferred_domain = BH_HMEM_DOMAIN_SYSTEM};
  bharat_hmem_handle_t h;
  void *p;
  assert(bh_hmem_create(&d, &h) == BH_OK);
  assert(bh_hmem_map_cpu(h, BH_HMEM_ACCESS_READ | BH_HMEM_ACCESS_WRITE, &p) ==
         BH_OK);
  assert(((uintptr_t)p & 63U) == 0U);
  for (unsigned i = 0; i < 128; i++)
    assert(((unsigned char *)p)[i] == 0U);
  assert(bh_hmem_map_cpu(h, BH_HMEM_ACCESS_READ, &p) == BH_ERR_BUSY);
  memset(p, 0xa5, 128);
  assert(bh_hmem_sync_for_device(h, 0, 128) == BH_OK);
  assert(bh_hmem_sync_for_cpu(h, 127, 2) == BH_ERR_INVALID_ARGUMENT);
  assert(bh_hmem_sync_for_cpu(h, UINT64_MAX, 2) == BH_ERR_INVALID_ARGUMENT);
  assert(bh_hmem_destroy(h) == BH_ERR_BUSY);
  assert(bh_hmem_unmap_cpu(h) == BH_OK);
  assert(bh_hmem_unmap_cpu(h) == BH_ERR_BAD_STATE);
  assert(bh_hmem_destroy(h) == BH_OK);
  assert(bh_hmem_destroy(h) == BH_ERR_NOT_FOUND);
  assert(bh_hmem_get_info(h, (bharat_hmem_info_v1_t *)&d) == BH_ERR_NOT_FOUND);
}
static void hmem_negative(void) {
  bharat_hmem_desc_v1_t d = {.version = 1,
                             .struct_size = sizeof(d),
                             .size = 1,
                             .alignment = 3,
                             .usage_flags = BH_HMEM_USAGE_CPU_READ,
                             .preferred_domain = BH_HMEM_DOMAIN_SYSTEM};
  bharat_hmem_handle_t h;
  assert(bh_hmem_create(&d, &h) == BH_ERR_INVALID_ARGUMENT);
  d.alignment = 16;
  d.size = 0;
  assert(bh_hmem_create(&d, &h) == BH_ERR_INVALID_ARGUMENT);
  d.size = 16;
  d.reserved[2] = 1;
  assert(bh_hmem_create(&d, &h) == BH_ERR_INVALID_ARGUMENT);
  d.reserved[2] = 0;
  d.preferred_domain = BH_HMEM_DOMAIN_DEVICE_LOCAL;
  assert(bh_hmem_create(&d, &h) == BH_ERR_UNSUPPORTED);
  d.preferred_domain = BH_HMEM_DOMAIN_HBM;
  assert(bh_hmem_create(&d, &h) == BH_ERR_UNSUPPORTED);
  d.preferred_domain = BH_HMEM_DOMAIN_MAX + 1U;
  assert(bh_hmem_create(&d, &h) == BH_ERR_INVALID_ARGUMENT);
}
static void tensors(void) {
  bh_tensor_desc_t d = {.dtype = BH_DTYPE_F32,
                        .rank = 2,
                        .shape = {4, 8},
                        .layout = BH_TENSOR_LAYOUT_DENSE};
  bh_tensor_t t, v;
  void *p;
  assert(bh_tensor_create(&d, &t) == BH_OK);
  assert(t.byte_length == 128 && t.stride[0] == 32 && t.stride[1] == 4);
  assert(bh_tensor_validate(&t) == BH_OK);
  assert(bh_hmem_map_cpu(t.memory, BH_HMEM_ACCESS_WRITE, &p) == BH_OK);
  ((float *)p)[0] = 1.0f;
  assert(bh_hmem_unmap_cpu(t.memory) == BH_OK);
  bh_tensor_view_desc_t vd = {.rank = 1,
                              .layout = BH_TENSOR_LAYOUT_STRIDED,
                              .shape = {8},
                              .stride = {4},
                              .byte_offset = 32};
  assert(bh_tensor_view(&t, &vd, &v) == BH_OK);
  assert(v.memory == t.memory && v.byte_offset == 32 && v.byte_length == 32);
  assert(bh_tensor_destroy(&v) == BH_OK);
  vd.byte_offset = 120;
  assert(bh_tensor_view(&t, &vd, &v) == BH_ERR_INVALID_ARGUMENT);
  assert(bh_tensor_destroy(&t) == BH_OK);
  assert(t.memory == 0);
}
static void tensor_negative(void) {
  bh_tensor_desc_t d = {.dtype = BH_DTYPE_F32,
                        .rank = 9,
                        .shape = {1},
                        .layout = BH_TENSOR_LAYOUT_DENSE};
  bh_tensor_t t;
  assert(bh_tensor_create(&d, &t) == BH_ERR_INVALID_ARGUMENT);
  d.rank = 2;
  d.shape[0] = UINT64_MAX;
  d.shape[1] = 2;
  assert(bh_tensor_create(&d, &t) == BH_ERR_OVERFLOW);
  d.rank = 0;
  d.dtype = BH_DTYPE_F32;
  assert(bh_tensor_create(&d, &t) == BH_OK);
  assert(t.byte_length == 4);
  assert(bh_tensor_destroy(&t) == BH_OK);
  d.rank = 1;
  d.dtype = BH_DTYPE_INVALID;
  d.shape[0] = 1;
  assert(bh_tensor_create(&d, &t) == BH_ERR_INVALID_ARGUMENT);
  d.dtype = BH_DTYPE_F32;
  d.layout = BH_TENSOR_LAYOUT_STRIDED;
  d.stride[0] = 2;
  assert(bh_tensor_create(&d, &t) == BH_ERR_INVALID_ARGUMENT);
}
int main(void) {
  hmem_lifecycle();
  hmem_negative();
  tensors();
  tensor_negative();
  return 0;
}
