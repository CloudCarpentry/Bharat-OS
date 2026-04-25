#ifndef BHARAT_SDK_NATIVE_H
#define BHARAT_SDK_NATIVE_H

#include <stdint.h>
#include <stddef.h>
#include <interface/uapi/capability/capability.h>
#include <interface/uapi/handle/handle.h>
#include <interface/uapi/ipc/ipc.h>
#include <interface/uapi/memory/memory.h>
#include <interface/uapi/time/time.h>
#include <interface/uapi/object/object.h>

/* Capability Management */
int bh_cap_revoke(bh_cap_t cap);
int bh_cap_transfer(bh_cap_t cap, bh_cap_t dest_ep);

/* Handle Management */
int bh_handle_close(bh_handle_t handle);
int bh_handle_query(bh_handle_t handle, struct bh_object_info* info);

/* IPC / uRPC */
int bh_ipc_call(bh_endpoint_t ep, struct bh_ipc_msg* msg);
int bh_ipc_send(bh_endpoint_t ep, struct bh_ipc_msg* msg);
int bh_ipc_recv(bh_endpoint_t ep, struct bh_ipc_msg* msg);

/* Memory / VM */
int bh_vm_map(void* addr, size_t size, uint32_t prot, uint32_t flags, bh_handle_t obj, uint64_t offset);
int bh_vm_unmap(void* addr, size_t size);
int bh_alloc_ex(size_t size, uint32_t mem_class, uint32_t flags, void** out_addr);

/* Time */
int bh_time_get(uint32_t clock_id, bh_time_t* out_time);
int bh_sleep_until(bh_deadline_t deadline);

/* Execution Intent */
int bh_thread_set_intent(bh_cap_t thread, const void* intent);

#endif /* BHARAT_SDK_NATIVE_H */
