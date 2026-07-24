#ifndef BHARAT_PERSONALITY_TYPES_H
#define BHARAT_PERSONALITY_TYPES_H

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Base types for OS personalities.
 *
 * Defines the core interfaces that a compatibility personality (Linux,
 * Windows, Android) must expose to the Bharat-OS subsystem manager.
 */

typedef enum {
    PERS_TYPE_NATIVE = 0,
    PERS_TYPE_LINUX = 1,
    PERS_TYPE_WINDOWS = 2,
    PERS_TYPE_ANDROID = 3
} personality_id_t;

/**
 * @brief Generic descriptor for a registered personality.
 */
typedef struct personality_desc {
    personality_id_t id;
    const char* name;

    // Callbacks provided by the personality to the core kernel
    int (*init)(struct personality_desc* self);
    int (*start)(struct personality_desc* self);
    int (*stop)(struct personality_desc* self);

    // Opaque context data for the personality
    void* context_data;
} personality_desc_t;

#endif // BHARAT_PERSONALITY_TYPES_H
