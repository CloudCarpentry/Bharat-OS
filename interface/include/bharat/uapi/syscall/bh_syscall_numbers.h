#ifndef BHARAT_UAPI_SYSCALL_NUMBERS_H
#define BHARAT_UAPI_SYSCALL_NUMBERS_H

/**
 * Bharat-OS Canonical Syscall Numbers
 *
 * ABI RULE: NEVER renumber, reorder, or reuse syscall numbers.
 * Append-only at the end of the table.
 */

#define BH_SYS_NOP                     0
#define BH_SYS_THREAD_CREATE           1
#define BH_SYS_THREAD_DESTROY          2
#define BH_SYS_SCHED_YIELD             3
#define BH_SYS_VMM_MAP_PAGE            4
#define BH_SYS_VMM_UNMAP_PAGE          5
#define BH_SYS_CAPABILITY_INVOKE       6
#define BH_SYS_ENDPOINT_CREATE         7
#define BH_SYS_ENDPOINT_SEND           8
#define BH_SYS_ENDPOINT_RECEIVE        9
#define BH_SYS_CAPABILITY_DELEGATE     10
#define BH_SYS_SCHED_SLEEP             11
#define BH_SYS_SCHED_SET_PRIORITY      12
#define BH_SYS_SCHED_SET_AFFINITY      13
#define BH_SYS_INTENT_SET              14
#define BH_SYS_INTENT_GET              15
#define BH_SYS_MEM_ALLOC_CLASS         16
#define BH_SYS_FAULT_DOMAIN_CREATE     17
#define BH_SYS_FAULT_DOMAIN_DESTROY    18
#define BH_SYS_FAULT_DOMAIN_ATTACH     19
#define BH_SYS_READ                    20
#define BH_SYS_WRITE                   21
#define BH_SYS_GET_SUBSYSTEM_CAPS      22
#define BH_SYS_THREAD_EXIT             23

#define BH_SYSCALL_COUNT               24

#endif /* BHARAT_UAPI_SYSCALL_NUMBERS_H */
