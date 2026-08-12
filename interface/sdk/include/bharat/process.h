#ifndef BHARAT_SDK_PROCESS_H
#define BHARAT_SDK_PROCESS_H
#include <bharat/types.h>
typedef void (*bh_thread_entry_t)(void *context);
typedef struct bh_process_options { uint32_t struct_size; uint32_t flags; bh_cap_t authority; uint32_t reserved; } bh_process_options_t;
typedef struct bh_thread_options { uint32_t struct_size; uint32_t flags; size_t stack_size; uint32_t reserved; } bh_thread_options_t;
bh_status_t bh_process_spawn(const char *path, const char *const argv[], const bh_process_options_t *options, bh_handle_t *process);
bh_status_t bh_process_wait(bh_handle_t process, uint64_t timeout_ns, int32_t *exit_status);
bh_status_t bh_thread_create(bh_thread_entry_t entry, void *context, const bh_thread_options_t *options, bh_handle_t *thread);
#endif
