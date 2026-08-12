#ifndef BHARAT_SDK_HMEM_H
#define BHARAT_SDK_HMEM_H

#include <bharat/memory/hmem.h>
#include <bharat/types.h>

bh_status_t bh_hmem_create(const bharat_hmem_desc_v1_t *desc,
                           bharat_hmem_handle_t *out);
bh_status_t bh_hmem_destroy(bharat_hmem_handle_t handle);
bh_status_t bh_hmem_get_info(bharat_hmem_handle_t handle,
                             bharat_hmem_info_v1_t *out);
bh_status_t bh_hmem_map_cpu(bharat_hmem_handle_t handle, uint32_t access,
                            void **out);
bh_status_t bh_hmem_unmap_cpu(bharat_hmem_handle_t handle);
bh_status_t bh_hmem_sync_for_cpu(bharat_hmem_handle_t handle, uint64_t offset,
                                 uint64_t length);
bh_status_t bh_hmem_sync_for_device(bharat_hmem_handle_t handle,
                                    uint64_t offset, uint64_t length);

#endif
