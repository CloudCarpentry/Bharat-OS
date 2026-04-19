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

// Initialize the tensor dispatch layer
int tensor_dispatch_init(void);

// Generic API for consumers to dispatch tensor ops
int tensor_process(tensor_op_t *op, const backend_dispatch_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_LIB_RUNTIME_TENSOR_BACKEND_H */
