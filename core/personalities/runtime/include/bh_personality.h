#ifndef BH_PERSONALITY_H
#define BH_PERSONALITY_H

#include <stdint.h>

/**
 * @brief OS Personality Kinds supported by Bharat-OS.
 * Matches BHARAT_PERSONALITY_* defines in UAPI where applicable.
 */
typedef enum bh_personality_kind {
    BH_PERSONALITY_NATIVE = 0,
    BH_PERSONALITY_LINUX = 1,
    BH_PERSONALITY_ANDROID = 2,
    BH_PERSONALITY_WINDOWS_NT = 3,
    BH_PERSONALITY_POSIX_LITE = 4,
    BH_PERSONALITY_AUTOMOTIVE = 5,
    BH_PERSONALITY_ROBOTICS = 6,
    BH_PERSONALITY_FREEBSD = 7,
    BH_PERSONALITY_WIN32 = 8,
    BH_PERSONALITY_DARWIN = 9,
    BH_PERSONALITY_MACOS = 10,
    BH_PERSONALITY_MAX = 31
} bh_personality_kind_t;

#endif // BH_PERSONALITY_H
