#include "console/console_core.h"
#include "lib/base/string.h"
#include "mm/hmem.h"
#include "slab.h"

#include <stdint.h>

#define BENCH_SIZE (64U * 1024U)
#define BENCH_ITERATIONS 100U
#define BENCH_HANDOFFS 3U
#define KPRINT(s) console_write_raw((s), string_length(s))

static void print_u64(uint64_t value) {
  char digits[21];
  uint32_t used = 0U;
  do {
    digits[used++] = (char)('0' + value % 10U);
    value /= 10U;
  } while (value != 0U);
  while (used != 0U)
    console_write_raw(&digits[--used], 1U);
}

static uint64_t checksum(const uint8_t *data, uint32_t size) {
  uint64_t hash = UINT64_C(1469598103934665603);
  for (uint32_t i = 0U; i < size; ++i) {
    hash ^= data[i];
    hash *= UINT64_C(1099511628211);
  }
  return hash;
}

static void emit(const char *key, uint64_t value) {
  KPRINT("BH_BENCH:");
  KPRINT(key);
  KPRINT("=");
  print_u64(value);
  KPRINT("\n");
}

/* Counters are local and deterministic; production HMEM hot paths remain
 * uninstrumented. The benchmark executes on the boot core with no sharing. */
void bh_hmem_benchmark_run(void) {
  uint8_t *buffers[BENCH_HANDOFFS + 1U] = {0};
  uint64_t baseline_hash = 0U;
  uint64_t hmem_hash = 0U;
  bharat_hmem_handle_t handle = 0U;
  uintptr_t mapped = 0U;
  bharat_hmem_desc_v1_t desc = {
      .version = BH_HMEM_DESC_VERSION_1,
      .struct_size = sizeof(bharat_hmem_desc_v1_t),
      .size = BENCH_SIZE,
      .alignment = 64U,
      .usage_flags = BH_HMEM_USAGE_CPU_READ | BH_HMEM_USAGE_CPU_WRITE,
      .property_flags = BH_HMEM_PROP_CPU_COHERENT,
      .preferred_domain = BH_HMEM_DOMAIN_SYSTEM,
  };
  const uint64_t rights =
      BH_HMEM_RIGHT_READ | BH_HMEM_RIGHT_WRITE | BH_HMEM_RIGHT_MAP_CPU;

  KPRINT("BH_BENCH:START\nBH_BENCH:TEST=PIPELINE_64K\n");
  emit("ITERATIONS", BENCH_ITERATIONS);
  for (uint32_t b = 0U; b <= BENCH_HANDOFFS; ++b) {
    buffers[b] = kmem_aligned_alloc(64U, BENCH_SIZE);
    if (buffers[b] == NULL)
      goto failed;
  }
  for (uint32_t iteration = 0U; iteration < BENCH_ITERATIONS; ++iteration) {
    for (uint32_t i = 0U; i < BENCH_SIZE; ++i)
      buffers[0][i] = (uint8_t)(i + iteration);
    for (uint32_t stage = 0U; stage < BENCH_HANDOFFS; ++stage)
      memcpy(buffers[stage + 1U], buffers[stage], BENCH_SIZE);
    baseline_hash ^= checksum(buffers[BENCH_HANDOFFS], BENCH_SIZE);
  }
  for (uint32_t b = 0U; b <= BENCH_HANDOFFS; ++b) {
    kmem_aligned_free(buffers[b]);
    buffers[b] = NULL;
  }
  if (bh_hmem_create(&desc, rights, &handle) != K_OK ||
      bh_hmem_map_cpu(handle, BH_HMEM_ACCESS_READ | BH_HMEM_ACCESS_WRITE,
                      &mapped) != K_OK)
    goto failed;
  for (uint32_t iteration = 0U; iteration < BENCH_ITERATIONS; ++iteration) {
    uint8_t *data = (uint8_t *)mapped;
    for (uint32_t i = 0U; i < BENCH_SIZE; ++i)
      data[i] = (uint8_t)(i + iteration);
    hmem_hash ^= checksum(data, BENCH_SIZE);
  }
  if (bh_hmem_unmap_cpu(handle) != K_OK || bh_hmem_destroy(handle) != K_OK)
    goto failed;
  emit("BASELINE_COPIES", BENCH_ITERATIONS * BENCH_HANDOFFS);
  emit("HMEM_COPIES", 0U);
  emit("BASELINE_BYTES_COPIED",
       (uint64_t)BENCH_SIZE * BENCH_ITERATIONS * BENCH_HANDOFFS);
  emit("HMEM_BYTES_COPIED", 0U);
  emit("BASELINE_ALLOCS", BENCH_HANDOFFS + 1U);
  emit("HMEM_OBJECTS", 1U);
  emit("HMEM_MAPS", 1U);
  emit("HMEM_UNMAPS", 1U);
  emit("BASELINE_CHECKSUM", baseline_hash);
  emit("HMEM_CHECKSUM", hmem_hash);
  KPRINT(baseline_hash == hmem_hash ? "BH_BENCH:RESULT=PASS\n"
                                    : "BH_BENCH:RESULT=FAIL\n");
  KPRINT("BH_BENCH:COMPLETE\n");
  return;
failed:
  for (uint32_t b = 0U; b <= BENCH_HANDOFFS; ++b)
    if (buffers[b] != NULL)
      kmem_aligned_free(buffers[b]);
  KPRINT("BH_BENCH:RESULT=FAIL\nBH_BENCH:COMPLETE\n");
}
