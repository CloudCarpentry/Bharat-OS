#ifndef BH_HANDLE_SPACE_H
#define BH_HANDLE_SPACE_H

/**
 * @brief Handle space kinds for different OS personalities.
 */
typedef enum bh_handle_space_kind {
    BH_HANDLE_SPACE_NATIVE = 0,
    BH_HANDLE_SPACE_POSIX_FD,
    BH_HANDLE_SPACE_NT_HANDLE,
    BH_HANDLE_SPACE_MACH_PORT,
    BH_HANDLE_SPACE_BINDER_HANDLE,
} bh_handle_space_kind_t;

#endif // BH_HANDLE_SPACE_H
