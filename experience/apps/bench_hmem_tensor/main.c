#define _POSIX_C_SOURCE 200809L
#include <bharat/compute/tensor.h>
#include <bharat/hmem.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PIPELINE_HANDOFFS UINT64_C(3)

typedef struct metrics {
  uint64_t elapsed_ns;
  uint64_t copies;
  uint64_t bytes_copied;
  uint64_t allocations;
  uint64_t deallocations;
  uint64_t maps;
  uint64_t unmaps;
  uint64_t views;
  uint64_t checksum;
} metrics_t;

static uint64_t now_ns(void) {
  struct timespec value;
  if (clock_gettime(CLOCK_MONOTONIC, &value) != 0)
    return 0U;
  return (uint64_t)value.tv_sec * UINT64_C(1000000000) +
         (uint64_t)value.tv_nsec;
}

static uint64_t checksum(const uint8_t *data, size_t size) {
  uint64_t hash = UINT64_C(1469598103934665603);
  for (size_t i = 0U; i < size; ++i) {
    hash ^= data[i];
    hash *= UINT64_C(1099511628211);
  }
  return hash;
}

static int baseline_pipeline(size_t size, uint64_t iterations, metrics_t *m) {
  uint8_t *buffers[PIPELINE_HANDOFFS + 1U] = {0};
  for (size_t i = 0U; i <= PIPELINE_HANDOFFS; ++i) {
    buffers[i] = malloc(size);
    if (buffers[i] == NULL)
      goto fail;
    ++m->allocations;
  }
  uint64_t start = now_ns();
  for (uint64_t iteration = 0U; iteration < iterations; ++iteration) {
    for (size_t i = 0U; i < size; ++i)
      buffers[0][i] = (uint8_t)(i + iteration);
    for (size_t stage = 0U; stage < PIPELINE_HANDOFFS; ++stage) {
      memcpy(buffers[stage + 1U], buffers[stage], size);
      ++m->copies;
      m->bytes_copied += size;
    }
    m->checksum ^= checksum(buffers[PIPELINE_HANDOFFS], size);
  }
  m->elapsed_ns = now_ns() - start;
  for (size_t i = 0U; i <= PIPELINE_HANDOFFS; ++i) {
    free(buffers[i]);
    ++m->deallocations;
  }
  return 0;
fail:
  for (size_t i = 0U; i <= PIPELINE_HANDOFFS; ++i)
    if (buffers[i] != NULL) {
      free(buffers[i]);
      ++m->deallocations;
    }
  return 1;
}

static int hmem_pipeline(size_t size, uint64_t iterations, metrics_t *m) {
  bharat_hmem_desc_v1_t desc = {
      .version = BH_HMEM_DESC_VERSION_1,
      .struct_size = sizeof(desc),
      .size = size,
      .alignment = 64U,
      .usage_flags = BH_HMEM_USAGE_CPU_READ | BH_HMEM_USAGE_CPU_WRITE,
      .property_flags = BH_HMEM_PROP_CPU_COHERENT,
      .preferred_domain = BH_HMEM_DOMAIN_SYSTEM,
  };
  bharat_hmem_handle_t handle;
  uint8_t *data;
  if (bh_hmem_create(&desc, &handle) != BH_OK)
    return 1;
  ++m->allocations;
  if (bh_hmem_map_cpu(handle, BH_HMEM_ACCESS_READ | BH_HMEM_ACCESS_WRITE,
                      (void **)&data) != BH_OK)
    return 1;
  ++m->maps;
  uint64_t start = now_ns();
  for (uint64_t iteration = 0U; iteration < iterations; ++iteration) {
    for (size_t i = 0U; i < size; ++i)
      data[i] = (uint8_t)(i + iteration);
    m->views += PIPELINE_HANDOFFS;
    m->checksum ^= checksum(data, size);
  }
  m->elapsed_ns = now_ns() - start;
  if (bh_hmem_unmap_cpu(handle) != BH_OK)
    return 1;
  ++m->unmaps;
  if (bh_hmem_destroy(handle) != BH_OK)
    return 1;
  ++m->deallocations;
  return 0;
}

static int tensor_view_benchmark(uint64_t iterations, metrics_t *copy,
                                 metrics_t *view) {
  bh_tensor_t tensor;
  bh_tensor_desc_t desc = {.dtype = BH_DTYPE_F32,
                           .layout = BH_TENSOR_LAYOUT_DENSE,
                           .rank = 2U,
                           .shape = {1024U, 1024U}};
  if (bh_tensor_create(&desc, &tensor) != BH_OK)
    return 1;
  for (uint64_t i = 0U; i < iterations; ++i) {
    void *slice = malloc(256U * 256U * sizeof(float));
    if (slice == NULL)
      return 1;
    memset(slice, (int)i, 256U * 256U * sizeof(float));
    ++copy->allocations;
    ++copy->copies;
    copy->bytes_copied += 256U * 256U * sizeof(float);
    copy->checksum ^= checksum(slice, 256U * 256U * sizeof(float));
    free(slice);
    ++copy->deallocations;

    bh_tensor_view_desc_t view_desc = {.rank = 2U,
                                       .layout = BH_TENSOR_LAYOUT_STRIDED,
                                       .shape = {256U, 256U},
                                       .stride = {4096U, 4U},
                                       .byte_offset = 0U};
    bh_tensor_t tensor_view;
    if (bh_tensor_view(&tensor, &view_desc, &tensor_view) != BH_OK)
      return 1;
    ++view->views;
    if (bh_tensor_destroy(&tensor_view) != BH_OK)
      return 1;
  }
  return bh_tensor_destroy(&tensor) == BH_OK ? 0 : 1;
}

static void emit_metric(const char *prefix, const char *key, uint64_t value) {
  printf("BH_BENCH:%s_%s=%" PRIu64 "\n", prefix, key, value);
}

int main(int argc, char **argv) {
  size_t size = 1024U * 1024U;
  uint64_t iterations = 100U;
  if (argc == 3) {
    size = (size_t)strtoull(argv[1], NULL, 10);
    iterations = strtoull(argv[2], NULL, 10);
  } else if (argc != 1) {
    fprintf(stderr, "usage: %s [bytes iterations]\n", argv[0]);
    return 2;
  }
  if (size == 0U || iterations == 0U) {
    fputs("bytes and iterations must be non-zero\n", stderr);
    return 2;
  }
  metrics_t baseline = {0}, hmem = {0}, copy = {0}, view = {0};
  puts("BH_BENCH:START\nBH_BENCH:TEST=SDK_HMEM_TENSOR");
  if (baseline_pipeline(size, iterations, &baseline) != 0 ||
      hmem_pipeline(size, iterations, &hmem) != 0 ||
      tensor_view_benchmark(iterations, &copy, &view) != 0)
    return 1;
  emit_metric("BASELINE", "TIME_NS", baseline.elapsed_ns);
  emit_metric("HMEM", "TIME_NS", hmem.elapsed_ns);
  emit_metric("BASELINE", "COPIES", baseline.copies);
  emit_metric("HMEM", "COPIES", hmem.copies);
  emit_metric("BASELINE", "BYTES_COPIED", baseline.bytes_copied);
  emit_metric("HMEM", "BYTES_COPIED", hmem.bytes_copied);
  emit_metric("BASELINE", "ALLOCS", baseline.allocations);
  emit_metric("HMEM", "ALLOCS", hmem.allocations);
  emit_metric("HMEM", "MAPS", hmem.maps);
  emit_metric("HMEM", "UNMAPS", hmem.unmaps);
  emit_metric("COPY_VIEW", "COPIES", copy.copies);
  emit_metric("COPY_VIEW", "BYTES_COPIED", copy.bytes_copied);
  emit_metric("COPY_VIEW", "ALLOCS", copy.allocations);
  emit_metric("TENSOR_VIEW", "COPIES", view.copies);
  emit_metric("TENSOR_VIEW", "BYTES_COPIED", view.bytes_copied);
  emit_metric("TENSOR_VIEW", "ALLOCS", view.allocations);
  emit_metric("TENSOR_VIEW", "VIEWS", view.views);
  emit_metric("BASELINE", "CHECKSUM", baseline.checksum);
  emit_metric("HMEM", "CHECKSUM", hmem.checksum);
  printf("BH_BENCH:RESULT=%s\nBH_BENCH:COMPLETE\n",
         baseline.checksum == hmem.checksum && hmem.copies == 0U &&
                 view.bytes_copied == 0U && view.allocations == 0U
             ? "PASS"
             : "FAIL");
  return baseline.checksum == hmem.checksum ? 0 : 1;
}
