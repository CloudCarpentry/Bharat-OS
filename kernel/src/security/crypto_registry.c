#include <stdint.h>
#include <lib/base/string.h>

#include <bharat/uapi/sys_errno.h>
#include <bharat/uapi/crypto/contract.h>
#include <security/crypto_caps.h>
#include <security/crypto_contract.h>
#include <security/crypto_registry.h>

/* Simple memory allocator stubs to avoid dragging in complex kernel mm */
extern void *kmalloc(size_t size);
extern void kfree(void *ptr);

#ifndef NULL
#define NULL ((void*)0)
#endif

/*
 * Note: A real kernel would use a spinlock here.
 * static spinlock_t registry_lock = SPINLOCK_INIT;
 */

static crypto_provider_node_t *registry_head = NULL;
static uint32_t next_provider_id = 1;

void crypto_registry_init(void)
{
    registry_head = NULL;
    next_provider_id = 1;
}

int crypto_provider_register(const crypto_provider_info_t *info, const crypto_provider_ops_t *ops)
{
    if (!info || !ops) {
        return -SYS_EINVAL;
    }
    if (info->backend_class != CRYPTO_BACKEND_CPU_ACCEL &&
        info->backend_class != CRYPTO_BACKEND_SECURE_ELEMENT_TPM &&
        info->backend_class != CRYPTO_BACKEND_RNG_PROVIDER) {
        return -SYS_EINVAL;
    }
    if (info->supported_ops_mask == 0U) {
        return -SYS_EINVAL;
    }
    if (info->name[0] == '\0') {
        return -SYS_EINVAL;
    }

    /* Ensure backend contract completeness for RNG providers. */
    if (info->backend_class == CRYPTO_BACKEND_RNG_PROVIDER && !ops->get_random) {
        return -SYS_EINVAL;
    }
    if (!ops->invoke && !ops->zeroize && !ops->get_random) {
        return -SYS_EINVAL;
    }

    crypto_provider_node_t *node = (crypto_provider_node_t *)kmalloc(sizeof(crypto_provider_node_t));
    if (!node) {
        return -SYS_ENOMEM;
    }

    /* Assign a unique ID and copy the info */
    node->id = next_provider_id++;
    memcpy(&node->info, info, sizeof(crypto_provider_info_t));
    node->ops = ops;
    node->next = NULL;

    /* Append to the list */
    if (!registry_head) {
        registry_head = node;
    } else {
        crypto_provider_node_t *curr = registry_head;
        while (curr->next) {
            curr = curr->next;
        }
        curr->next = node;
    }

    return node->id;
}

int crypto_provider_unregister(uint32_t provider_id)
{
    crypto_provider_node_t *curr = registry_head;
    crypto_provider_node_t *prev = NULL;

    while (curr) {
        if (curr->id == provider_id) {
            if (prev) {
                prev->next = curr->next;
            } else {
                registry_head = curr->next;
            }
            kfree(curr);
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }

    return -SYS_ENOENT;
}

const crypto_provider_node_t* crypto_provider_find(uint32_t provider_id)
{
    crypto_provider_node_t *curr = registry_head;
    while (curr) {
        if (curr->id == provider_id) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

int crypto_provider_invoke(uint32_t provider_id, crypto_op_args_t *args)
{
    if (!args) {
        return -SYS_EINVAL;
    }

    /* 1. Capability check BEFORE finding the provider */
    uint32_t required_cap = 0;
    switch (args->op) {
        case CRYPTO_OP_GET_RANDOM:
            required_cap = CAP_TYPE_RNG;
            break;
        case CRYPTO_OP_SEAL:
        case CRYPTO_OP_UNSEAL:
            required_cap = CAP_TYPE_SEALER;
            break;
        case CRYPTO_OP_HASH_BUFFER:
        case CRYPTO_OP_ZEROIZE_KEY:
            required_cap = CAP_TYPE_CRYPTO_KEY; /* Assume they need key caps for these */
            break;
        default:
            return -SYS_EINVAL;
    }

    int cap_err = crypto_cap_check(required_cap);
    if (cap_err != 0) {
        return cap_err; /* -SYS_EPERM */
    }

    /* 2. Find the provider */
    const crypto_provider_node_t *node = crypto_provider_find(provider_id);
    if (!node) {
        return -SYS_ENODEV;
    }

    /* 3. Check if the provider supports this operation */
    if (!(node->info.supported_ops_mask & (1 << args->op))) {
        return -SYS_ENOSYS;
    }

    /* 4. Dispatch the provider operation */
    if (args->op == CRYPTO_OP_ZEROIZE_KEY) {
        if (!args->input_buf || args->input_len == 0) {
            return -SYS_EINVAL;
        }
        if (node->ops && node->ops->zeroize) {
            node->ops->zeroize(args->input_buf, args->input_len);
            return 0;
        }
        return -SYS_ENOSYS;
    }

    if (node->ops && node->ops->invoke) {
        return node->ops->invoke(args);
    }

    return -SYS_ENOSYS;
}

int crypto_get_random(void *buf, size_t len)
{
    if (!buf || len == 0) return -SYS_EINVAL;

    int cap_err = crypto_cap_check(CAP_TYPE_RNG);
    if (cap_err != 0) {
        return cap_err;
    }

    crypto_provider_node_t *curr = registry_head;
    while (curr) {
        if (curr->info.backend_class == CRYPTO_BACKEND_RNG_PROVIDER && curr->ops && curr->ops->get_random) {
            return curr->ops->get_random(buf, len);
        }
        curr = curr->next;
    }

    return -SYS_ENODEV;
}

void crypto_zeroize_buffer(void *buf, size_t len)
{
    if (buf && len > 0) {
        /*
         * In a real kernel, this would use a compiler-specific built-in
         * or inline assembly to ensure it's not optimized away.
         * For this C implementation, a standard memset is used as a placeholder.
         */
        volatile unsigned char *p = (volatile unsigned char *)buf;
        while (len--) {
            *p++ = 0;
        }
    }
}
