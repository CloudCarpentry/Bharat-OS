#ifndef BHARAT_UAPI_SYSCALL_NR_H
#define BHARAT_UAPI_SYSCALL_NR_H

/* Thread management */
#define SYSCALL_THREAD_CREATE        1
#define SYSCALL_THREAD_DESTROY       2
#define SYSCALL_THREAD_YIELD         3
#define SYSCALL_THREAD_SLEEP         4
#define SYSCALL_THREAD_SET_PRIORITY  5
#define SYSCALL_THREAD_SET_AFFINITY  6
#define SYSCALL_THREAD_EXIT          7

/* Memory management */
#define SYSCALL_MEM_ALLOC            10
#define SYSCALL_MEM_FREE             11
#define SYSCALL_MEM_MAP              12
#define SYSCALL_MEM_UNMAP            13
#define SYSCALL_MEM_PROTECT          14

/* Capability management */
#define SYSCALL_CAP_REVOKE           20
#define SYSCALL_CAP_TRANSFER         21
#define SYSCALL_CAP_DERIVE           22

/* IPC */
#define SYSCALL_IPC_CALL             30
#define SYSCALL_IPC_SEND             31
#define SYSCALL_IPC_RECV             32

/* Time */
#define SYSCALL_TIME_GET             40
#define SYSCALL_TIME_SLEEP           41

/* Objects/Handles */
#define SYSCALL_HANDLE_CLOSE         50
#define SYSCALL_HANDLE_QUERY         51

/* Fault Domains */
#define SYSCALL_FAULT_DOMAIN_CREATE  60
#define SYSCALL_FAULT_DOMAIN_ATTACH  61

#endif /* BHARAT_UAPI_SYSCALL_NR_H */
