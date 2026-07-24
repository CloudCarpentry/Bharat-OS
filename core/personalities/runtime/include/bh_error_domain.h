#ifndef BH_ERROR_DOMAIN_H
#define BH_ERROR_DOMAIN_H

/**
 * @brief Error domains for different OS personalities.
 */
typedef enum bh_error_domain {
    BH_ERROR_DOMAIN_NATIVE = 0,
    BH_ERROR_DOMAIN_POSIX,
    BH_ERROR_DOMAIN_LINUX,
    BH_ERROR_DOMAIN_BSD,
    BH_ERROR_DOMAIN_NTSTATUS,
    BH_ERROR_DOMAIN_WIN32,
    BH_ERROR_DOMAIN_MACH,
    BH_ERROR_DOMAIN_DARWIN,
} bh_error_domain_t;

#endif // BH_ERROR_DOMAIN_H
