#include "../../include/mm/address_token.h"
#include "../../include/security/isolation.h"
#include <stddef.h>

int bharat_addr_token_validate(const bharat_addr_token_t* token,
                               uint64_t addr,
                               uint64_t size,
                               uint32_t required_flags) {
    if (!token) {
        return -1; // EINVAL
    }

    if (!(token->flags & ADDR_TOKEN_FLAG_VALID)) {
        return -2; // EINVAL
    }

    if (token->flags & ADDR_TOKEN_FLAG_REVOKED) {
        return -3; // EPERM
    }

    // TODO: implement expiry checks using global kernel ticks if expiry_ticks > 0
    if (token->flags & ADDR_TOKEN_FLAG_EXPIRED) {
        return -4; // EPERM
    }

    if ((token->flags & required_flags) != required_flags) {
        return -5; // EACCES
    }

    if (addr < token->resource_base || (addr + size) > (token->resource_base + token->resource_size)) {
        return -6; // ERANGE
    }

    return 0; // Authorized
}

void bharat_addr_token_revoke(bharat_addr_token_t* token) {
    if (token) {
        token->flags |= ADDR_TOKEN_FLAG_REVOKED;
        token->flags &= ~ADDR_TOKEN_FLAG_VALID;
    }
}
