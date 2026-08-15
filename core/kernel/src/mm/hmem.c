#include "mm/hmem.h"
#include "lib/base/string.h"
#include "slab.h"
#include "spinlock.h"

#define HMEM_SLOT_COUNT 64U

typedef struct bh_hmem_device_mapping {
  bh_device_id_t device;
  uint64_t device_address;
  uint64_t visible_generation;
  uint32_t access;
  uint32_t flags;
} bh_hmem_device_mapping_t;

typedef struct bh_hmem {
  uint64_t object_id;
  uint32_t generation;
  uint32_t slot;
  uint64_t size;
  uint64_t alignment;
  uint64_t usage_flags;
  uint64_t property_flags;
  uint64_t rights;
  uint32_t preferred_domain;
  uint32_t cpu_map_count;
  void *cpu_va;
  uint64_t content_generation;
  bh_hmem_state_t state;
  spinlock_t lock;
  bh_hmem_device_mapping_t mappings[BH_HMEM_MAX_DEVICE_MAPPINGS];
} bh_hmem_t;

/* Transitional lock-protected shared registry. Slots are bounded; a later
 * per-core object registry migration must preserve the generation-bearing
 * handle contract. */
static bh_hmem_t *hmem_slots[HMEM_SLOT_COUNT];
static uint32_t hmem_generations[HMEM_SLOT_COUNT];
static spinlock_t hmem_registry_lock;
static uint64_t hmem_next_object_id = 1U;

static bool power_of_two(uint64_t value) {
  return value != 0U && (value & (value - 1U)) == 0U;
}
static bharat_hmem_handle_t make_handle(uint32_t slot, uint32_t generation) {
  return ((uint64_t)generation << 32) | (uint64_t)(slot + 1U);
}

static kstatus_t lookup_locked(bharat_hmem_handle_t handle, bh_hmem_t **out) {
  uint32_t encoded_slot = (uint32_t)handle;
  uint32_t generation = (uint32_t)(handle >> 32);
  if (encoded_slot == 0U || encoded_slot > HMEM_SLOT_COUNT || generation == 0U)
    return K_ERR_CAP_INVALID;
  bh_hmem_t *object = hmem_slots[encoded_slot - 1U];
  if (object == NULL)
    return hmem_generations[encoded_slot - 1U] == generation ? K_ERR_NOT_FOUND
                                                             : K_ERR_CAP_STALE;
  if (object->generation != generation)
    return K_ERR_CAP_STALE;
  if (object->state != BH_HMEM_LIVE)
    return K_ERR_BAD_STATE;
  *out = object;
  return K_OK;
}

static kstatus_t validate_desc(const bharat_hmem_desc_v1_t *desc) {
  uint32_t i;
  if (desc == NULL || desc->version != BH_HMEM_DESC_VERSION_1 ||
      desc->struct_size != sizeof(*desc) || desc->size == 0U)
    return K_ERR_INVALID_ARG;
  if (!power_of_two(desc->alignment) || desc->alignment < sizeof(void *) ||
      desc->alignment > (uint64_t)(size_t)-1)
    return K_ERR_ALIGNMENT;
  if (desc->size > (uint64_t)(size_t)-1 ||
      desc->size > (uint64_t)(size_t)-1 - desc->alignment - sizeof(void *))
    return K_ERR_OVERFLOW;
  if ((desc->usage_flags & ~BH_HMEM_USAGE_ALL) != 0U ||
      (desc->property_flags & ~BH_HMEM_PROP_ALL) != 0U ||
      desc->preferred_domain > BH_HMEM_DOMAIN_MAX || desc->reserved0 != 0U)
    return K_ERR_INVALID_ARG;
  for (i = 0U; i < 4U; ++i)
    if (desc->reserved[i] != 0U)
      return K_ERR_INVALID_ARG;
  if (desc->preferred_domain != BH_HMEM_DOMAIN_SYSTEM &&
      desc->preferred_domain != BH_HMEM_DOMAIN_SHARED)
    return K_ERR_UNSUPPORTED;
  return K_OK;
}

kstatus_t bh_hmem_create(const bharat_hmem_desc_v1_t *desc, uint64_t rights,
                         bharat_hmem_handle_t *out) {
  bh_hmem_t *object;
  uint32_t slot;
  kstatus_t status = validate_desc(desc);
  if (status != K_OK || out == NULL)
    return status != K_OK ? status : K_ERR_INVALID_ARG;
  object = kmem_aligned_alloc(desc->alignment, sizeof(*object));
  if (object == NULL)
    return K_ERR_NO_MEMORY;
  memset(object, 0, sizeof(*object));
  object->cpu_va = kmem_aligned_alloc(desc->alignment, (size_t)desc->size);
  if (object->cpu_va == NULL) {
    kmem_aligned_free(object);
    return K_ERR_NO_MEMORY;
  }
  if ((desc->property_flags & BH_HMEM_PROP_ZERO_ON_ALLOC) != 0U)
    memset(object->cpu_va, 0, (size_t)desc->size);
  spin_lock_init(&object->lock);
  object->size = desc->size;
  object->alignment = desc->alignment;
  object->usage_flags = desc->usage_flags;
  object->property_flags = desc->property_flags;
  object->preferred_domain = desc->preferred_domain;
  object->rights = rights;
  object->content_generation = 1U;
  object->state = BH_HMEM_LIVE;
  spin_lock(&hmem_registry_lock);
  for (slot = 0U; slot < HMEM_SLOT_COUNT && hmem_slots[slot] != NULL; ++slot) {
  }
  if (slot == HMEM_SLOT_COUNT) {
    spin_unlock(&hmem_registry_lock);
    kmem_aligned_free(object->cpu_va);
    kmem_aligned_free(object);
    return K_ERR_NO_RESOURCES;
  }
  if (++hmem_generations[slot] == 0U)
    ++hmem_generations[slot];
  object->slot = slot;
  object->generation = hmem_generations[slot];
  object->object_id = hmem_next_object_id++;
  hmem_slots[slot] = object;
  *out = make_handle(slot, object->generation);
  spin_unlock(&hmem_registry_lock);
  return K_OK;
}

kstatus_t bh_hmem_destroy(bharat_hmem_handle_t handle) {
  bh_hmem_t *object;
  kstatus_t status;
  spin_lock(&hmem_registry_lock);
  status = lookup_locked(handle, &object);
  if (status != K_OK) {
    spin_unlock(&hmem_registry_lock);
    return status;
  }
  spin_lock(&object->lock);
  if (object->cpu_map_count != 0U) {
    spin_unlock(&object->lock);
    spin_unlock(&hmem_registry_lock);
    return K_ERR_BUSY;
  }
  for (uint32_t i = 0U; i < BH_HMEM_MAX_DEVICE_MAPPINGS; ++i)
    if (object->mappings[i].flags != 0U) {
      spin_unlock(&object->lock);
      spin_unlock(&hmem_registry_lock);
      return K_ERR_BUSY;
    }
  object->state = BH_HMEM_DYING;
  hmem_slots[object->slot] = NULL;
  spin_unlock(&object->lock);
  spin_unlock(&hmem_registry_lock);
  if ((object->property_flags & BH_HMEM_PROP_ZERO_ON_FREE) != 0U)
    secure_memzero(object->cpu_va, (size_t)object->size);
  object->state = BH_HMEM_DEAD;
  kmem_aligned_free(object->cpu_va);
  kmem_aligned_free(object);
  return K_OK;
}

kstatus_t bh_hmem_get_info(bharat_hmem_handle_t handle,
                           bharat_hmem_info_v1_t *out) {
  bh_hmem_t *o;
  kstatus_t s;
  if (out == NULL)
    return K_ERR_INVALID_ARG;
  spin_lock(&hmem_registry_lock);
  s = lookup_locked(handle, &o);
  if (s == K_OK) {
    memset(out, 0, sizeof(*out));
    out->version = 1U;
    out->struct_size = sizeof(*out);
    out->size = o->size;
    out->alignment = o->alignment;
    out->usage_flags = o->usage_flags;
    out->property_flags = o->property_flags;
    out->content_generation = o->content_generation;
    out->preferred_domain = o->preferred_domain;
    out->cpu_map_count = o->cpu_map_count;
  }
  spin_unlock(&hmem_registry_lock);
  return s;
}

kstatus_t bh_hmem_map_cpu(bharat_hmem_handle_t handle, uint32_t access,
                          uintptr_t *out_addr) {
  bh_hmem_t *o;
  kstatus_t s;
  if (out_addr == NULL || access == 0U ||
      (access & ~(BH_HMEM_ACCESS_READ | BH_HMEM_ACCESS_WRITE |
                  BH_HMEM_ACCESS_EXECUTE)) != 0U)
    return K_ERR_INVALID_ARG;
  spin_lock(&hmem_registry_lock);
  s = lookup_locked(handle, &o);
  if (s == K_OK) {
    if ((o->rights & BH_HMEM_RIGHT_MAP_CPU) == 0U ||
        ((access & BH_HMEM_ACCESS_READ) != 0U &&
         (o->rights & BH_HMEM_RIGHT_READ) == 0U) ||
        ((access & BH_HMEM_ACCESS_WRITE) != 0U &&
         (o->rights & BH_HMEM_RIGHT_WRITE) == 0U))
      s = K_ERR_CAP_DENIED;
    else {
      spin_lock(&o->lock);
      if (o->cpu_map_count != 0U)
        s = K_ERR_BUSY;
      else {
        o->cpu_map_count = 1U;
        *out_addr = (uintptr_t)o->cpu_va;
      }
      spin_unlock(&o->lock);
    }
  }
  spin_unlock(&hmem_registry_lock);
  return s;
}

kstatus_t bh_hmem_unmap_cpu(bharat_hmem_handle_t handle) {
  bh_hmem_t *o;
  kstatus_t s;
  spin_lock(&hmem_registry_lock);
  s = lookup_locked(handle, &o);
  if (s == K_OK) {
    spin_lock(&o->lock);
    if (o->cpu_map_count == 0U)
      s = K_ERR_BAD_STATE;
    else {
      o->cpu_map_count = 0U;
      o->content_generation++;
    }
    spin_unlock(&o->lock);
  }
  spin_unlock(&hmem_registry_lock);
  return s;
}

static kstatus_t sync_range(bharat_hmem_handle_t h, uint64_t off, uint64_t len,
                            hal_hmem_sync_direction_t dir) {
  bh_hmem_t *o;
  kstatus_t s;
  spin_lock(&hmem_registry_lock);
  s = lookup_locked(h, &o);
  if (s == K_OK) {
    if (len == 0U || off > o->size || len > o->size - off)
      s = K_ERR_INVALID_ARG;
    else if ((dir == HAL_HMEM_SYNC_TO_CPU &&
              (o->rights & BH_HMEM_RIGHT_READ) == 0U) ||
             (dir == HAL_HMEM_SYNC_TO_DEVICE &&
              (o->rights & BH_HMEM_RIGHT_WRITE) == 0U))
      s = K_ERR_CAP_DENIED;
    else if ((o->property_flags & BH_HMEM_PROP_CPU_COHERENT) != 0U) {
      __atomic_thread_fence(__ATOMIC_SEQ_CST);
      s = K_OK;
    } else
      s = hal_hmem_sync_range((uintptr_t)o->cpu_va + (uintptr_t)off,
                              (size_t)len, dir);
  }
  spin_unlock(&hmem_registry_lock);
  return s;
}
kstatus_t bh_hmem_sync_for_cpu(bharat_hmem_handle_t h, uint64_t o, uint64_t l) {
  return sync_range(h, o, l, HAL_HMEM_SYNC_TO_CPU);
}
kstatus_t bh_hmem_sync_for_device(bharat_hmem_handle_t h, uint64_t o,
                                  uint64_t l) {
  return sync_range(h, o, l, HAL_HMEM_SYNC_TO_DEVICE);
}

kstatus_t bh_hmem_map_device(bharat_hmem_handle_t h, bh_device_id_t d,
                             uint32_t a, uint64_t *out) {
  bh_hmem_t *o;
  kstatus_t s;
  if (out == NULL)
    return K_ERR_INVALID_ARG;
  spin_lock(&hmem_registry_lock);
  s = lookup_locked(h, &o);
  if (s == K_OK) {
    if ((o->rights & BH_HMEM_RIGHT_MAP_DEVICE) == 0U)
      s = K_ERR_CAP_DENIED;
    else {
      hal_hmem_device_map_req_t r = {d, (uintptr_t)o->cpu_va, o->size, a, 0U};
      hal_hmem_device_map_result_t x;
      s = hal_hmem_map_device(&r, &x);
      if (s == K_OK) {
        uint32_t i;
        for (i = 0;
             i < BH_HMEM_MAX_DEVICE_MAPPINGS && o->mappings[i].flags != 0U;
             i++) {
        }
        if (i == BH_HMEM_MAX_DEVICE_MAPPINGS)
          s = K_ERR_NO_RESOURCES;
        else {
          o->mappings[i] = (bh_hmem_device_mapping_t){
              d, x.device_address, o->content_generation, a, 1U};
          *out = x.device_address;
        }
      }
    }
  }
  spin_unlock(&hmem_registry_lock);
  return s;
}
kstatus_t bh_hmem_unmap_device(bharat_hmem_handle_t h, bh_device_id_t d) {
  bh_hmem_t *o;
  kstatus_t s;
  spin_lock(&hmem_registry_lock);
  s = lookup_locked(h, &o);
  if (s == K_OK) {
    uint32_t i;
    for (i = 0; i < BH_HMEM_MAX_DEVICE_MAPPINGS &&
                (o->mappings[i].flags == 0U || o->mappings[i].device != d);
         i++) {
    }
    if (i == BH_HMEM_MAX_DEVICE_MAPPINGS)
      s = K_ERR_NOT_FOUND;
    else {
      hal_hmem_device_unmap_req_t r = {d, o->mappings[i].device_address,
                                       o->size};
      s = hal_hmem_unmap_device(&r);
      if (s == K_OK)
        memset(&o->mappings[i], 0, sizeof(o->mappings[i]));
    }
  }
  spin_unlock(&hmem_registry_lock);
  return s;
}
