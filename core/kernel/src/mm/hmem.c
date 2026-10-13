#include "mm/hmem.h"
#include "hal/hal.h"
#include "lib/base/string.h"
#include "mm/physmap.h"
#include "slab.h"
#include "spinlock.h"

#define HMEM_MAX_CORES 16U
#define HMEM_SLOTS_PER_CORE 32U

/*
 * Per-core HMEM object registry.
 * Each core operates independently on its own slice of object slots,
 * preventing cross-core lock contention on allocations and lookups.
 */
static bh_hmem_t *hmem_core_slots[HMEM_MAX_CORES][HMEM_SLOTS_PER_CORE];
static uint32_t hmem_core_generations[HMEM_MAX_CORES][HMEM_SLOTS_PER_CORE];
static spinlock_t hmem_core_locks[HMEM_MAX_CORES];
static uint32_t hmem_next_object_id = 1U;

static bool power_of_two(uint64_t value) {
  return value != 0U && (value & (value - 1U)) == 0U;
}

static inline bharat_hmem_handle_t make_handle(uint32_t core, uint32_t slot, uint32_t generation) {
  uint32_t encoded_slot = ((core & 0xFFU) << 16) | ((slot + 1U) & 0xFFFFU);
  return ((uint64_t)generation << 32) | (uint64_t)encoded_slot;
}

kstatus_t bh_hmem_lookup(bharat_hmem_handle_t handle, bh_hmem_t **out) {
  if (out == NULL) return K_ERR_INVALID_ARG;

  uint32_t encoded_slot = (uint32_t)handle;
  uint32_t generation = (uint32_t)(handle >> 32);
  uint32_t core = (encoded_slot >> 16) & 0xFFU;
  uint32_t slot_idx = (encoded_slot & 0xFFFFU);

  if (slot_idx == 0U || slot_idx > HMEM_SLOTS_PER_CORE || core >= HMEM_MAX_CORES || generation == 0U) {
    return K_ERR_CAP_INVALID;
  }
  uint32_t slot = slot_idx - 1U;

  spin_lock(&hmem_core_locks[core]);
  bh_hmem_t *object = hmem_core_slots[core][slot];
  if (object == NULL) {
    kstatus_t ret = (hmem_core_generations[core][slot] == generation) ? K_ERR_NOT_FOUND : K_ERR_CAP_STALE;
    spin_unlock(&hmem_core_locks[core]);
    return ret;
  }
  if (object->generation != generation) {
    spin_unlock(&hmem_core_locks[core]);
    return K_ERR_CAP_STALE;
  }
  if (object->state != BH_HMEM_LIVE) {
    spin_unlock(&hmem_core_locks[core]);
    return K_ERR_BAD_STATE;
  }
  *out = object;
  spin_unlock(&hmem_core_locks[core]);
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

kstatus_t bh_hmem_create_object(const bharat_hmem_desc_v1_t *desc, uint64_t rights,
                                uint32_t owner_core, bh_hmem_t **out_obj) {
  kstatus_t status = validate_desc(desc);
  if (status != K_OK || out_obj == NULL) {
    return status != K_OK ? status : K_ERR_INVALID_ARG;
  }

  size_t page_count = ((size_t)desc->size + PAGE_SIZE - 1U) / PAGE_SIZE;
  if (page_count == 0U) return K_ERR_INVALID_ARG;

  uint32_t pmm_flags = PMM_ALLOC_CONTIGUOUS;
  if ((desc->property_flags & BH_HMEM_PROP_ZERO_ON_ALLOC) != 0U) {
    pmm_flags |= PMM_ALLOC_ZERO;
  }
  if ((desc->property_flags & BH_HMEM_PROP_PINNED) != 0U) {
    pmm_flags |= PMM_ALLOC_PINNED;
  }

  pmm_block_t block;
  int pmm_ret = pmm_alloc_contiguous_ex((uint32_t)page_count, PMM_ZONE_ANY, MEM_DMA, pmm_flags, &block);
  if (pmm_ret != 0) {
    return K_ERR_NO_MEMORY;
  }

  void *kernel_va = physmap_phys_to_virt(block.phys_addr);
  if ((desc->property_flags & BH_HMEM_PROP_ZERO_ON_ALLOC) != 0U && kernel_va != NULL) {
    memset(kernel_va, 0, (size_t)desc->size);
  }

  vm_object_t *vm_obj = vm_object_create_dma(block.phys_addr, (size_t)(page_count * PAGE_SIZE), 0, 0, 0);
  if (vm_obj == NULL) {
    pmm_free_pages(&block);
    return K_ERR_NO_MEMORY;
  }

  bh_hmem_t *object = (bh_hmem_t *)kmalloc(sizeof(bh_hmem_t));
  if (object == NULL) {
    vm_object_release(vm_obj);
    pmm_free_pages(&block);
    return K_ERR_NO_MEMORY;
  }

  memset(object, 0, sizeof(*object));
  spin_lock_init(&object->lock);
  object->size = desc->size;
  object->alignment = desc->alignment;
  object->usage_flags = desc->usage_flags;
  object->property_flags = desc->property_flags;
  object->preferred_domain = desc->preferred_domain;
  object->rights = rights;
  object->pmm_block = block;
  object->kernel_mapping = kernel_va;
  object->vm_obj = vm_obj;
  object->content_generation = 1U;
  object->state = BH_HMEM_LIVE;

  if (owner_core >= HMEM_MAX_CORES) {
    owner_core = 0U;
  }
  object->owner_core_id = owner_core;

  spin_lock(&hmem_core_locks[owner_core]);
  uint32_t slot;
  for (slot = 0U; slot < HMEM_SLOTS_PER_CORE && hmem_core_slots[owner_core][slot] != NULL; ++slot) {
  }
  if (slot == HMEM_SLOTS_PER_CORE) {
    spin_unlock(&hmem_core_locks[owner_core]);
    vm_object_release(vm_obj);
    pmm_free_pages(&block);
    kfree(object);
    return K_ERR_NO_RESOURCES;
  }

  if (++hmem_core_generations[owner_core][slot] == 0U) {
    ++hmem_core_generations[owner_core][slot];
  }
  object->slot = slot;
  object->generation = hmem_core_generations[owner_core][slot];
  object->object_id = (uint64_t)__atomic_fetch_add(&hmem_next_object_id, 1U, __ATOMIC_RELAXED);
  hmem_core_slots[owner_core][slot] = object;
  spin_unlock(&hmem_core_locks[owner_core]);

  *out_obj = object;
  return K_OK;
}

kstatus_t bh_hmem_create(const bharat_hmem_desc_v1_t *desc, uint64_t rights,
                         bharat_hmem_handle_t *out) {
  if (out == NULL) return K_ERR_INVALID_ARG;

  uint32_t core = hal_cpu_get_id();
  if (core >= HMEM_MAX_CORES) core = 0U;

  bh_hmem_t *object = NULL;
  kstatus_t status = bh_hmem_create_object(desc, rights, core, &object);
  if (status != K_OK) return status;

  *out = make_handle(core, object->slot, object->generation);
  return K_OK;
}

kstatus_t bh_hmem_destroy_object(bh_hmem_t *object) {
  if (object == NULL) return K_ERR_INVALID_ARG;

  spin_lock(&object->lock);
  if (object->cpu_map_count != 0U) {
    spin_unlock(&object->lock);
    return K_ERR_BUSY;
  }
  for (uint32_t i = 0U; i < BH_HMEM_MAX_DEVICE_MAPPINGS; ++i) {
    if (object->mappings[i].flags != 0U) {
      spin_unlock(&object->lock);
      return K_ERR_BUSY;
    }
  }
  object->state = BH_HMEM_DYING;

  uint32_t core = object->owner_core_id;
  if (core >= HMEM_MAX_CORES) core = 0U;

  spin_lock(&hmem_core_locks[core]);
  if (object->slot < HMEM_SLOTS_PER_CORE) {
    hmem_core_slots[core][object->slot] = NULL;
  }
  spin_unlock(&hmem_core_locks[core]);
  spin_unlock(&object->lock);

  if ((object->property_flags & BH_HMEM_PROP_ZERO_ON_FREE) != 0U && object->kernel_mapping != NULL) {
    secure_memzero(object->kernel_mapping, (size_t)object->size);
  }
  object->state = BH_HMEM_DEAD;

  if (object->vm_obj != NULL) {
    vm_object_release(object->vm_obj);
    object->vm_obj = NULL;
  }
  pmm_free_pages(&object->pmm_block);
  kfree(object);
  return K_OK;
}

kstatus_t bh_hmem_destroy(bharat_hmem_handle_t handle) {
  bh_hmem_t *object = NULL;
  kstatus_t status = bh_hmem_lookup(handle, &object);
  if (status != K_OK) return status;
  return bh_hmem_destroy_object(object);
}

kstatus_t bh_hmem_get_info(bharat_hmem_handle_t handle,
                           bharat_hmem_info_v1_t *out) {
  bh_hmem_t *o;
  if (out == NULL) return K_ERR_INVALID_ARG;

  kstatus_t s = bh_hmem_lookup(handle, &o);
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
  return s;
}

kstatus_t bh_hmem_map_cpu(bharat_hmem_handle_t handle, uint32_t access,
                          uintptr_t *out_addr) {
  bh_hmem_t *o;
  if (out_addr == NULL || access == 0U ||
      (access & ~(BH_HMEM_ACCESS_READ | BH_HMEM_ACCESS_WRITE |
                  BH_HMEM_ACCESS_EXECUTE)) != 0U) {
    return K_ERR_INVALID_ARG;
  }
  if ((access & BH_HMEM_ACCESS_WRITE) != 0U && (access & BH_HMEM_ACCESS_EXECUTE) != 0U) {
    return K_ERR_DENIED;
  }

  kstatus_t s = bh_hmem_lookup(handle, &o);
  if (s == K_OK) {
    if ((o->rights & BH_HMEM_RIGHT_MAP_CPU) == 0U ||
        ((access & BH_HMEM_ACCESS_READ) != 0U &&
         (o->rights & BH_HMEM_RIGHT_READ) == 0U) ||
        ((access & BH_HMEM_ACCESS_WRITE) != 0U &&
         (o->rights & BH_HMEM_RIGHT_WRITE) == 0U)) {
      return K_ERR_CAP_DENIED;
    }

    spin_lock(&o->lock);
    if (o->cpu_map_count != 0U) {
      spin_unlock(&o->lock);
      return K_ERR_BUSY;
    }
    o->cpu_map_count = 1U;
    *out_addr = (uintptr_t)o->kernel_mapping;
    spin_unlock(&o->lock);
  }
  return s;
}

kstatus_t bh_hmem_unmap_cpu(bharat_hmem_handle_t handle) {
  bh_hmem_t *o;
  kstatus_t s = bh_hmem_lookup(handle, &o);
  if (s == K_OK) {
    spin_lock(&o->lock);
    if (o->cpu_map_count == 0U) {
      spin_unlock(&o->lock);
      return K_ERR_BAD_STATE;
    }
    o->cpu_map_count = 0U;
    o->content_generation++;
    spin_unlock(&o->lock);
  }
  return s;
}

static kstatus_t sync_range(bharat_hmem_handle_t h, uint64_t off, uint64_t len,
                            hal_hmem_sync_direction_t dir) {
  bh_hmem_t *o;
  kstatus_t s = bh_hmem_lookup(h, &o);
  if (s == K_OK) {
    if (len == 0U || off > o->size || len > o->size - off) {
      return K_ERR_INVALID_ARG;
    }
    if ((dir == HAL_HMEM_SYNC_TO_CPU && (o->rights & BH_HMEM_RIGHT_READ) == 0U) ||
        (dir == HAL_HMEM_SYNC_TO_DEVICE && (o->rights & BH_HMEM_RIGHT_WRITE) == 0U)) {
      return K_ERR_CAP_DENIED;
    }
    if ((o->property_flags & BH_HMEM_PROP_CPU_COHERENT) != 0U) {
      __atomic_thread_fence(__ATOMIC_SEQ_CST);
      return K_OK;
    }
    s = hal_hmem_sync_range((uintptr_t)o->kernel_mapping + (uintptr_t)off,
                            (size_t)len, dir);
  }
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
  if (out == NULL) return K_ERR_INVALID_ARG;

  kstatus_t s = bh_hmem_lookup(h, &o);
  if (s == K_OK) {
    if ((o->rights & BH_HMEM_RIGHT_MAP_DEVICE) == 0U) {
      return K_ERR_CAP_DENIED;
    }
    hal_hmem_device_map_req_t r = {d, (uintptr_t)o->kernel_mapping, o->size, a, 0U};
    hal_hmem_device_map_result_t x;
    s = hal_hmem_map_device(&r, &x);
    if (s == K_OK) {
      spin_lock(&o->lock);
      uint32_t i;
      for (i = 0; i < BH_HMEM_MAX_DEVICE_MAPPINGS && o->mappings[i].flags != 0U; i++) {
      }
      if (i == BH_HMEM_MAX_DEVICE_MAPPINGS) {
        spin_unlock(&o->lock);
        return K_ERR_NO_RESOURCES;
      }
      o->mappings[i] = (bh_hmem_device_mapping_t){
          d, x.device_address, o->content_generation, a, 1U};
      *out = x.device_address;
      spin_unlock(&o->lock);
    }
  }
  return s;
}

kstatus_t bh_hmem_unmap_device(bharat_hmem_handle_t h, bh_device_id_t d) {
  bh_hmem_t *o;
  kstatus_t s = bh_hmem_lookup(h, &o);
  if (s == K_OK) {
    spin_lock(&o->lock);
    uint32_t i;
    for (i = 0; i < BH_HMEM_MAX_DEVICE_MAPPINGS &&
                (o->mappings[i].flags == 0U || o->mappings[i].device != d);
         i++) {
    }
    if (i == BH_HMEM_MAX_DEVICE_MAPPINGS) {
      spin_unlock(&o->lock);
      return K_ERR_NOT_FOUND;
    }
    uint64_t dev_addr = o->mappings[i].device_address;
    memset(&o->mappings[i], 0, sizeof(o->mappings[i]));
    spin_unlock(&o->lock);

    hal_hmem_device_unmap_req_t r = {d, dev_addr, o->size};
    s = hal_hmem_unmap_device(&r);
  }
  return s;
}
