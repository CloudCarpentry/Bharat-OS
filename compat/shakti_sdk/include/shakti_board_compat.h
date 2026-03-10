#ifndef BHARAT_COMPAT_SHAKTI_BOARD_COMPAT_H
#define BHARAT_COMPAT_SHAKTI_BOARD_COMPAT_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Compatibility identifiers for SHAKTI-style board selection.
 * Internal Bharat-OS code should continue using Bharat-native board/profile APIs.
 */
typedef enum {
    SHAKTI_COMPAT_BOARD_UNKNOWN = 0,
    SHAKTI_COMPAT_BOARD_E,
    SHAKTI_COMPAT_BOARD_C,
    SHAKTI_COMPAT_BOARD_I,
} shakti_compat_board_t;

shakti_compat_board_t shakti_compat_current_board(void);

#ifdef __cplusplus
}
#endif

#endif
