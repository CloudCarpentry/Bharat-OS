#ifndef BHARAT_ANDROID_BOOT_H
#define BHARAT_ANDROID_BOOT_H

#include "android_personality.h"

/*
 * Phase 1/Phase 2 Boot Sequence Hooks
 *
 * Manages the initialization sequence of the Android personality:
 * Kernel Boot -> Subsystem Manager -> Android Personality Init ->
 * Android init -> Zygote -> System Server.
 */

/**
 * @brief Early Android Subsystem boot hook.
 *
 * Sets up global namespaces, the distributed Binder servicemanager root,
 * and preallocates per-core structures required for Android tasks.
 */
int android_boot_early_init(android_personality_t* p);

/**
 * @brief Launch Android `init` process.
 *
 * Translates the Android-compatible `init` executable loading and maps
 * it to a Bharat-OS process with the Android personality tag.
 *
 * @param init_path Path to the init binary (e.g. "/system/bin/init").
 * @param p The Android personality descriptor.
 * @return 0 on success, < 0 on error.
 */
int android_boot_start_init(android_personality_t* p, const char* init_path);

/**
 * @brief Hook triggered when the Android Service Manager starts.
 *
 * The core kernel maps the userspace Service Manager to the distributed
 * Binder logical root node.
 */
int android_boot_on_servicemanager_ready(void);

#endif // BHARAT_ANDROID_BOOT_H
