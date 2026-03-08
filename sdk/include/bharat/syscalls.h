#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Stub for system calls in user-space
void bharat_exit(int code);
int bharat_write(int fd, const void* buf, size_t count);
int bharat_get_subsystem_caps(uint32_t* storage_caps, uint32_t* network_caps);

#ifdef __cplusplus
}
#endif
