#define _POSIX_C_SOURCE 200112L
#include <bharat/hmem.h>
#include <stdlib.h>
#include <string.h>

#define HOST_HMEM_SLOTS 64U
typedef struct {
  void *data;
  uint64_t size, alignment, usage, properties, generation;
  uint32_t domain, mapped, slot_generation;
} host_hmem_t;
static host_hmem_t slots[HOST_HMEM_SLOTS];
static volatile unsigned char registry_lock;
static void lock_registry(void) {
  while (__atomic_test_and_set(&registry_lock, __ATOMIC_ACQUIRE)) {
  }
}
static void unlock_registry(void) {
  __atomic_clear(&registry_lock, __ATOMIC_RELEASE);
}
static int pow2(uint64_t x) { return x != 0U && (x & (x - 1U)) == 0U; }
static host_hmem_t *lookup(bharat_hmem_handle_t h) {
  uint32_t s = (uint32_t)h, g = (uint32_t)(h >> 32);
  if (s == 0U || s > HOST_HMEM_SLOTS || g == 0U)
    return NULL;
  host_hmem_t *o = &slots[s - 1U];
  return o->data != NULL && o->slot_generation == g ? o : NULL;
}
static int reserved_zero(const bharat_hmem_desc_v1_t *d) {
  for (unsigned i = 0; i < 4; i++)
    if (d->reserved[i])
      return 0;
  return d->reserved0 == 0U;
}
bh_status_t bh_hmem_create(const bharat_hmem_desc_v1_t *d,
                           bharat_hmem_handle_t *out) {
  void *p = NULL;
  uint32_t i;
  if (!d || !out || d->version != 1U || d->struct_size != sizeof(*d) ||
      d->size == 0U || !pow2(d->alignment) || d->alignment < sizeof(void *) ||
      d->size > SIZE_MAX - d->alignment || !reserved_zero(d))
    return BH_ERR_INVALID_ARGUMENT;
  if ((d->usage_flags & ~BH_HMEM_USAGE_ALL) ||
      (d->property_flags & ~BH_HMEM_PROP_ALL) ||
      d->preferred_domain > BH_HMEM_DOMAIN_SECURE)
    return BH_ERR_INVALID_ARGUMENT;
  if (d->preferred_domain != BH_HMEM_DOMAIN_SYSTEM &&
      d->preferred_domain != BH_HMEM_DOMAIN_SHARED)
    return BH_ERR_UNSUPPORTED;
  if (posix_memalign(&p, (size_t)d->alignment, (size_t)d->size) != 0)
    return BH_ERR_NO_MEMORY;
  if (d->property_flags & BH_HMEM_PROP_ZERO_ON_ALLOC)
    memset(p, 0, (size_t)d->size);
  lock_registry();
  for (i = 0; i < HOST_HMEM_SLOTS && slots[i].data; i++) {
  }
  if (i == HOST_HMEM_SLOTS) {
    unlock_registry();
    free(p);
    return BH_ERR_BUSY;
  }
  uint32_t g = slots[i].slot_generation + 1U;
  if (g == 0U)
    g = 1U;
  slots[i] = (host_hmem_t){p,
                           d->size,
                           d->alignment,
                           d->usage_flags,
                           d->property_flags,
                           1U,
                           d->preferred_domain,
                           0U,
                           g};
  *out = ((uint64_t)g << 32) | (i + 1U);
  unlock_registry();
  return BH_OK;
}
bh_status_t bh_hmem_destroy(bharat_hmem_handle_t h) {
  lock_registry();
  host_hmem_t *o = lookup(h);
  if (!o) {
    unlock_registry();
    return BH_ERR_NOT_FOUND;
  }
  if (o->mapped) {
    unlock_registry();
    return BH_ERR_BUSY;
  }
  void *p = o->data;
  uint64_t n = o->size, props = o->properties;
  uint32_t g = o->slot_generation;
  memset(o, 0, sizeof(*o));
  o->slot_generation = g;
  unlock_registry();
  if (props & BH_HMEM_PROP_ZERO_ON_FREE) {
    volatile unsigned char *v = p;
    while (n--)
      *v++ = 0;
  }
  free(p);
  return BH_OK;
}
bh_status_t bh_hmem_get_info(bharat_hmem_handle_t h,
                             bharat_hmem_info_v1_t *out) {
  if (!out)
    return BH_ERR_INVALID_ARGUMENT;
  lock_registry();
  host_hmem_t *o = lookup(h);
  if (!o) {
    unlock_registry();
    return BH_ERR_NOT_FOUND;
  }
  memset(out, 0, sizeof(*out));
  out->version = 1;
  out->struct_size = sizeof(*out);
  out->size = o->size;
  out->alignment = o->alignment;
  out->usage_flags = o->usage;
  out->property_flags = o->properties;
  out->content_generation = o->generation;
  out->preferred_domain = o->domain;
  out->cpu_map_count = o->mapped;
  unlock_registry();
  return BH_OK;
}
bh_status_t bh_hmem_map_cpu(bharat_hmem_handle_t h, uint32_t a, void **out) {
  if (!out || a == 0U ||
      (a &
       ~(BH_HMEM_ACCESS_READ | BH_HMEM_ACCESS_WRITE | BH_HMEM_ACCESS_EXECUTE)))
    return BH_ERR_INVALID_ARGUMENT;
  lock_registry();
  host_hmem_t *o = lookup(h);
  if (!o) {
    unlock_registry();
    return BH_ERR_NOT_FOUND;
  }
  if (((a & BH_HMEM_ACCESS_READ) && !(o->usage & BH_HMEM_USAGE_CPU_READ)) ||
      ((a & BH_HMEM_ACCESS_WRITE) && !(o->usage & BH_HMEM_USAGE_CPU_WRITE)) ||
      ((a & BH_HMEM_ACCESS_EXECUTE) && !(o->usage & BH_HMEM_USAGE_EXECUTE))) {
    unlock_registry();
    return BH_ERR_ACCESS_DENIED;
  }
  if (o->mapped) {
    unlock_registry();
    return BH_ERR_BUSY;
  }
  o->mapped = 1;
  *out = o->data;
  unlock_registry();
  return BH_OK;
}
bh_status_t bh_hmem_unmap_cpu(bharat_hmem_handle_t h) {
  lock_registry();
  host_hmem_t *o = lookup(h);
  if (!o) {
    unlock_registry();
    return BH_ERR_NOT_FOUND;
  }
  if (!o->mapped) {
    unlock_registry();
    return BH_ERR_BAD_STATE;
  }
  o->mapped = 0;
  o->generation++;
  unlock_registry();
  return BH_OK;
}
static bh_status_t sync_hmem(bharat_hmem_handle_t h, uint64_t off,
                             uint64_t len) {
  lock_registry();
  host_hmem_t *o = lookup(h);
  if (!o) {
    unlock_registry();
    return BH_ERR_NOT_FOUND;
  }
  if (len == 0U || off > o->size || len > o->size - off) {
    unlock_registry();
    return BH_ERR_INVALID_ARGUMENT;
  }
  __atomic_thread_fence(__ATOMIC_SEQ_CST);
  unlock_registry();
  return BH_OK;
}
bh_status_t bh_hmem_sync_for_cpu(bharat_hmem_handle_t h, uint64_t o,
                                 uint64_t l) {
  return sync_hmem(h, o, l);
}
bh_status_t bh_hmem_sync_for_device(bharat_hmem_handle_t h, uint64_t o,
                                    uint64_t l) {
  return sync_hmem(h, o, l);
}
