#ifndef BHARAT_PROFILES_PERSONALITY_ENV_H
#define BHARAT_PROFILES_PERSONALITY_ENV_H

#include "bharat_config.h"

/*
 * Personality Profiles
 * Controls higher-level compatibility behavior, conventions, and UX semantics.
 */

#if defined(BHARAT_PERSONALITY_LINUX)
#define PERSONALITY_PROFILE_NAME "Linux-like"
#define PATH_SEPARATOR '/'
#define SYS_API_STYLE "POSIX"

#elif defined(BHARAT_PERSONALITY_WINDOWS)
#define PERSONALITY_PROFILE_NAME "Windows-like"
#define PATH_SEPARATOR '\\'
#define SYS_API_STYLE "Win32"

#elif defined(BHARAT_PERSONALITY_MAC)
#define PERSONALITY_PROFILE_NAME "Mac-like"
#define PATH_SEPARATOR '/'
#define SYS_API_STYLE "Darwin/BSD"

#else
#define PERSONALITY_PROFILE_NAME "Native/None"
#define PATH_SEPARATOR '/'
#define SYS_API_STYLE "Microkernel Native"

#endif

#endif /* BHARAT_PROFILES_PERSONALITY_ENV_H */
