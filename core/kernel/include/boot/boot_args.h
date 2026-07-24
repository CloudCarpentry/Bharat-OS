#ifndef BHARAT_BOOT_ARGS_H
#define BHARAT_BOOT_ARGS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Initialize the boot command line string.
 * This should be called early in the boot process by architecture-specific setup.
 *
 * @param cmdline A null-terminated string containing the boot arguments.
 */
void boot_args_init(const char *cmdline);

/**
 * @brief Get the entire boot command line string.
 *
 * @return const char* Pointer to the command line string, or empty string if none.
 */
const char* boot_get_cmdline(void);

/**
 * @brief Check if a specific flag exists in the boot arguments (e.g., "debug").
 *
 * @param key The flag to search for.
 * @return true if the flag is present, false otherwise.
 */
bool boot_has_flag(const char* key);

/**
 * @brief Get the value associated with a key in the boot arguments (e.g., "test=quick").
 *
 * @param key The key to search for (e.g., "test").
 * @param out Buffer to store the value.
 * @param out_sz Size of the output buffer.
 * @return true if the key was found and value copied, false otherwise.
 */
bool boot_get_kv(const char* key, char* out, size_t out_sz);

#endif // BHARAT_BOOT_ARGS_H
