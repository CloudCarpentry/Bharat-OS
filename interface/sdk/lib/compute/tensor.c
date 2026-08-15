#include <bharat/compute/tensor.h>
#include <string.h>

static uint64_t dtype_size(uint32_t d) {
  switch (d) {
  case BH_DTYPE_F32:
  case BH_DTYPE_I32:
    return 4;
  case BH_DTYPE_F16:
  case BH_DTYPE_BF16:
  case BH_DTYPE_I16:
    return 2;
  case BH_DTYPE_I8:
  case BH_DTYPE_U8:
  case BH_DTYPE_BOOL:
    return 1;
  default:
    return 0;
  }
}
static int mul(uint64_t a, uint64_t b, uint64_t *out) {
  if (a && b > UINT64_MAX / a)
    return 0;
  *out = a * b;
  return 1;
}
static bh_status_t tensor_bytes(uint32_t dtype, uint32_t rank,
                                const uint64_t *shape, uint64_t *out) {
  uint64_t n = 1, s = dtype_size(dtype);
  if (!s || rank > BH_TENSOR_MAX_RANK)
    return BH_ERR_INVALID_ARGUMENT;
  for (uint32_t i = 0; i < rank; i++) {
    if (shape[i] == 0U)
      return BH_ERR_INVALID_ARGUMENT;
    if (!mul(n, shape[i], &n))
      return BH_ERR_OVERFLOW;
  }
  if (!mul(n, s, out))
    return BH_ERR_OVERFLOW;
  return BH_OK;
}
static bh_status_t validate_strides(uint32_t layout, uint32_t rank,
                                    const uint64_t *shape,
                                    const uint64_t *stride, uint64_t elem,
                                    uint64_t *span) {
  uint64_t expected = elem, max = elem;
  if (layout > BH_TENSOR_LAYOUT_STRIDED)
    return BH_ERR_INVALID_ARGUMENT;
  for (uint32_t i = rank; i > 0; i--) {
    uint32_t j = i - 1;
    if (stride[j] < elem)
      return BH_ERR_INVALID_ARGUMENT;
    if (layout == BH_TENSOR_LAYOUT_DENSE && stride[j] != expected)
      return BH_ERR_INVALID_ARGUMENT;
    uint64_t add;
    if (!mul(shape[j] - 1U, stride[j], &add) || max > UINT64_MAX - add)
      return BH_ERR_OVERFLOW;
    max += add;
    if (!mul(expected, shape[j], &expected))
      return BH_ERR_OVERFLOW;
  }
  *span = max;
  return BH_OK;
}
bh_status_t bh_tensor_validate(const bh_tensor_t *t) {
  bharat_hmem_info_v1_t info;
  uint64_t bytes, span, elem;
  if (!t || t->memory == 0U || t->rank > BH_TENSOR_MAX_RANK ||
      ((t->flags & BH_TENSOR_F_OWNS_MEMORY) && (t->flags & BH_TENSOR_F_VIEW)))
    return BH_ERR_INVALID_ARGUMENT;
  bh_status_t s = tensor_bytes(t->dtype, t->rank, t->shape, &bytes);
  if (s != BH_OK)
    return s;
  elem = dtype_size(t->dtype);
  s = validate_strides(t->layout, t->rank, t->shape, t->stride, elem, &span);
  if (s != BH_OK)
    return s;
  if (t->byte_length < span || t->byte_offset > UINT64_MAX - t->byte_length)
    return BH_ERR_INVALID_ARGUMENT;
  s = bh_hmem_get_info(t->memory, &info);
  if (s != BH_OK)
    return s;
  if (t->byte_offset > info.size || t->byte_length > info.size - t->byte_offset)
    return BH_ERR_INVALID_ARGUMENT;
  (void)bytes;
  return BH_OK;
}
bh_status_t bh_tensor_create(const bh_tensor_desc_t *d, bh_tensor_t *out) {
  uint64_t bytes, elem;
  if (!d || !out || d->reserved || d->rank > BH_TENSOR_MAX_RANK)
    return BH_ERR_INVALID_ARGUMENT;
  bh_status_t s = tensor_bytes(d->dtype, d->rank, d->shape, &bytes);
  if (s != BH_OK)
    return s;
  memset(out, 0, sizeof(*out));
  out->rank = d->rank;
  out->dtype = d->dtype;
  out->layout = d->layout;
  out->flags = BH_TENSOR_F_OWNS_MEMORY;
  memcpy(out->shape, d->shape, sizeof(out->shape));
  elem = dtype_size(d->dtype);
  uint64_t next = elem;
  for (uint32_t i = d->rank; i > 0; i--) {
    uint32_t j = i - 1;
    out->stride[j] =
        d->layout == BH_TENSOR_LAYOUT_STRIDED ? d->stride[j] : next;
    if (!mul(next, d->shape[j], &next))
      return BH_ERR_OVERFLOW;
  }
  if (d->rank == 0U)
    out->stride[0] = 0U;
  out->byte_length = bytes;
  bharat_hmem_desc_v1_t hd = {
      .version = 1,
      .struct_size = sizeof(hd),
      .size = bytes,
      .alignment = sizeof(void *) < 16U ? 16U : sizeof(void *),
      .usage_flags = BH_HMEM_USAGE_CPU_READ | BH_HMEM_USAGE_CPU_WRITE |
                     BH_HMEM_USAGE_DEVICE_READ | BH_HMEM_USAGE_DEVICE_WRITE,
      .property_flags = d->hmem_property_flags | BH_HMEM_PROP_ZERO_ON_ALLOC,
      .preferred_domain = BH_HMEM_DOMAIN_SYSTEM};
  s = bh_hmem_create(&hd, &out->memory);
  if (s != BH_OK) {
    memset(out, 0, sizeof(*out));
    return s;
  }
  s = bh_tensor_validate(out);
  if (s != BH_OK) {
    bh_hmem_destroy(out->memory);
    memset(out, 0, sizeof(*out));
  }
  return s;
}
bh_status_t bh_tensor_destroy(bh_tensor_t *t) {
  if (!t)
    return BH_ERR_INVALID_ARGUMENT;
  bh_status_t s = BH_OK;
  if ((t->flags & BH_TENSOR_F_OWNS_MEMORY) && t->memory)
    s = bh_hmem_destroy(t->memory);
  if (s == BH_OK)
    memset(t, 0, sizeof(*t));
  return s;
}
bh_status_t bh_tensor_view(const bh_tensor_t *p, const bh_tensor_view_desc_t *v,
                           bh_tensor_t *out) {
  if (!p || !v || !out || v->rank > BH_TENSOR_MAX_RANK)
    return BH_ERR_INVALID_ARGUMENT;
  bh_status_t s = bh_tensor_validate(p);
  if (s != BH_OK)
    return s;
  memset(out, 0, sizeof(*out));
  out->memory = p->memory;
  out->rank = v->rank;
  out->dtype = p->dtype;
  out->layout = v->layout;
  out->flags = BH_TENSOR_F_VIEW;
  memcpy(out->shape, v->shape, sizeof(out->shape));
  memcpy(out->stride, v->stride, sizeof(out->stride));
  if (p->byte_offset > UINT64_MAX - v->byte_offset)
    return BH_ERR_OVERFLOW;
  out->byte_offset = p->byte_offset + v->byte_offset;
  uint64_t span;
  s = validate_strides(out->layout, out->rank, out->shape, out->stride,
                       dtype_size(out->dtype), &span);
  if (s != BH_OK)
    return s;
  out->byte_length = span;
  return bh_tensor_validate(out);
}
