#ifndef BHARAT_LIB_RUNTIME_TENSOR_BACKEND_H
#define BHARAT_LIB_RUNTIME_TENSOR_BACKEND_H

#include "../backend_dispatch/backend_dispatch.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TENSOR_OP_MATMUL,
    TENSOR_OP_CONV2D,
    TENSOR_OP_RELU
} tensor_op_type_t;

typedef struct {
    tensor_op_type_t op;
    const float *input_tensor;
    size_t in_elements;
    float *output_tensor;
    size_t out_elements;
} tensor_op_t;

struct backend_interface {
    int (*process_tensor)(tensor_op_t *op);
};

typedef struct {
    uint64_t backend_hw_selected;
    uint64_t backend_sw_fallback;
    uint64_t jobs_completed;
    uint64_t jobs_failed;

    uint64_t safe_mode_fallbacks;
    uint64_t unavailable_fallbacks;
} tensor_dispatch_stats_t;

typedef struct {
    const char *backend_name;
    backend_type_t backend_type;
    int execution_status;
} tensor_dispatch_result_t;

// Initialize the tensor dispatch layer and reset registry/stats
int tensor_dispatch_init(void);

// Generic API for consumers to select tensor backend
const backend_provider_t *tensor_select_backend(
    const bharat_hw_caps_t *caps,
    const backend_dispatch_context_t *ctx);

// Generic API for consumers to dispatch tensor ops
int tensor_process(
    tensor_op_t *op,
    const bharat_hw_caps_t *caps,
    const backend_dispatch_context_t *ctx);

// Snapshot the dispatch stats
void tensor_dispatch_get_stats(tensor_dispatch_stats_t *out);

// Retrieve the last dispatch result
void tensor_dispatch_get_last_result(tensor_dispatch_result_t *out);

// Reset statistics specifically (useful for isolated tests)
void tensor_dispatch_reset_stats(void);

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_LIB_RUNTIME_TENSOR_BACKEND_H */
