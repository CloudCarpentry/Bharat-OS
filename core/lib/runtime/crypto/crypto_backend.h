#ifndef BHARAT_LIB_RUNTIME_CRYPTO_BACKEND_H
#define BHARAT_LIB_RUNTIME_CRYPTO_BACKEND_H

#include "../backend_dispatch/backend_dispatch.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CRYPTO_OP_ENCRYPT,
    CRYPTO_OP_DECRYPT,
    CRYPTO_OP_HASH
} crypto_op_type_t;

typedef struct {
    crypto_op_type_t op;
    const uint8_t *in;
    size_t in_len;
    uint8_t *out;
    size_t out_len;
} crypto_op_t;

// The opaque interface for a crypto backend implementation
struct backend_interface {
    int (*process_op)(crypto_op_t *op);
};

// Initialize the crypto dispatch layer
int crypto_dispatch_init(void);

// Generic API for consumers to use the optimal crypto backend
int crypto_process(crypto_op_t *op, const backend_dispatch_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_LIB_RUNTIME_CRYPTO_BACKEND_H */
