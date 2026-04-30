#ifndef BHARAT_KERNEL_STATUS_H
#define BHARAT_KERNEL_STATUS_H

#include <stdint.h>
#include <bharat/uapi/sys_errno.h>

/*
 * Bharat-OS Internal Kernel Status Codes
 * Defines the rich internal error and status architecture for the kernel.
 * These are strictly internal and must be translated via kstatus_to_sysret()
 * before returning to userspace via syscalls.
 */

typedef int32_t kstatus_t;

/* Helper macros */
#define K_OK  0
#define KSTATUS_FAMILY(s)  (((-(int32_t)(s)) & 0xFFFFFF00) >> 8)
#define KSTATUS_IS_OK(s)   ((s) == K_OK)

/* ── Generic / Common ─────────── 0 .. -255 */
#define K_ERR_INVALID_ARG       ((kstatus_t)-1)
#define K_ERR_BAD_STATE         ((kstatus_t)-2)
#define K_ERR_REQUIRES_FS_SERVICE ((kstatus_t)-31)
#define K_ERR_NOT_FOUND         ((kstatus_t)-3)
#define K_ERR_ALREADY_EXISTS    ((kstatus_t)-4)
#define K_ERR_UNSUPPORTED       ((kstatus_t)-5)
#define K_ERR_BUSY              ((kstatus_t)-6)
#define K_ERR_TIMEOUT           ((kstatus_t)-7)
#define K_ERR_RETRY             ((kstatus_t)-8)
#define K_ERR_CANCELLED         ((kstatus_t)-9)
#define K_ERR_PARTIAL           ((kstatus_t)-10)
#define K_ERR_OVERFLOW          ((kstatus_t)-11)
#define K_ERR_CHECKSUM          ((kstatus_t)-12)
#define K_ERR_AGAIN             ((kstatus_t)-13) /* transient, retry immediately */
#define K_ERR_INTERRUPTED       ((kstatus_t)-14) /* preempted/interrupted */
#define K_ERR_FAULT             ((kstatus_t)-15)
#define K_ERR_DENIED            ((kstatus_t)-16)
#define K_ERR_INTERNAL_BUG      ((kstatus_t)-17)
#define K_ERR_INVALID_SYSCALL   ((kstatus_t)-18)
#define K_ERR_IN_PROGRESS       ((kstatus_t)-19)

/* ── Memory / VMM / PMM ───────── -256 .. -511 */
#define K_ERR_NO_MEMORY         ((kstatus_t)-256)
#define K_ERR_VM_UNMAPPED       ((kstatus_t)-257)
#define K_ERR_VM_ALREADY_MAPPED ((kstatus_t)-258)
#define K_ERR_VM_PROT           ((kstatus_t)-259)
#define K_ERR_PMM_EXHAUSTED     ((kstatus_t)-260)
#define K_ERR_DMA_BOUNDARY      ((kstatus_t)-261)
#define K_ERR_ALIGNMENT         ((kstatus_t)-262)

/* ── Scheduler / Thread ───────── -512 .. -767 */
#define K_ERR_NO_TASK           ((kstatus_t)-512)
#define K_ERR_BAD_THREAD        ((kstatus_t)-513)
#define K_ERR_NOT_RUNNABLE      ((kstatus_t)-514)
#define K_ERR_ISOLATED          ((kstatus_t)-519)
#define K_ERR_ALREADY_BLOCKED   ((kstatus_t)-515)
#define K_ERR_WRONG_AFFINITY    ((kstatus_t)-516)
#define K_ERR_DEADLINE_MISS     ((kstatus_t)-517)
#define K_ERR_QUOTA_EXCEEDED    ((kstatus_t)-518)

/* ── Capability / Security ────── -768 .. -1023 */
#define K_ERR_CAP_INVALID       ((kstatus_t)-768)
#define K_ERR_CAP_WRONG_TYPE    ((kstatus_t)-769)
#define K_ERR_CAP_REVOKED       ((kstatus_t)-770)
#define K_ERR_CAP_DENIED        ((kstatus_t)-771)
#define K_ERR_CAP_OWNERSHIP     ((kstatus_t)-772)
#define K_ERR_LABEL_VIOLATION   ((kstatus_t)-773)
#define K_ERR_SANDBOX_VIOLATION ((kstatus_t)-774)
#define K_ERR_CAP_STALE         ((kstatus_t)-775)

/* ── IPC / URPC ───────────────── -1024 .. -1279 */
#define K_ERR_IPC_NO_ENDPOINT   ((kstatus_t)-1024)
#define K_ERR_IPC_CLOSED        ((kstatus_t)-1025)
#define K_ERR_IPC_QUEUE_FULL    ((kstatus_t)-1026)
#define K_ERR_IPC_PEER_DEAD     ((kstatus_t)-1027)
#define K_ERR_IPC_MSG_TOO_LARGE ((kstatus_t)-1028)
#define K_ERR_IPC_REMOTE_UNAVAIL ((kstatus_t)-1029)
#define K_ERR_IPC_ROUTE_FAILED  ((kstatus_t)-1030)
#define K_ERR_IPC_DELEGATION    ((kstatus_t)-1031)
#define K_ERR_IPC_CROSS_CORE    ((kstatus_t)-1032)

/* ── Filesystem / VFS ─────────── -1280 .. -1535 */
#define K_ERR_VFS_NOT_MOUNTED   ((kstatus_t)-1280)
#define K_ERR_VFS_READ_ONLY     ((kstatus_t)-1281)
#define K_ERR_VFS_NOT_DIR       ((kstatus_t)-1282)
#define K_ERR_VFS_IS_DIR        ((kstatus_t)-1283)
#define K_ERR_VFS_OUT_OF_SPACE  ((kstatus_t)-1284)
#define K_ERR_VFS_CORRUPTED     ((kstatus_t)-1285)

/* ── Device / Driver ──────────── -1536 .. -1791 */
#define K_ERR_DEV_NO_DEVICE     ((kstatus_t)-1536)
#define K_ERR_DEV_NOT_READY     ((kstatus_t)-1537)
#define K_ERR_DEV_OFFLINE       ((kstatus_t)-1538)
#define K_ERR_DEV_DMA_FAIL      ((kstatus_t)-1539)
#define K_ERR_DEV_MMIO_FAULT    ((kstatus_t)-1540)
#define K_ERR_DEV_RESET_REQD    ((kstatus_t)-1541)

/* ── Network ──────────────────── -1792 .. -2047 */
#define K_ERR_NET_IF_DOWN       ((kstatus_t)-1792)
#define K_ERR_NET_NO_ROUTE      ((kstatus_t)-1793)
#define K_ERR_NET_BAD_ADDR      ((kstatus_t)-1794)
#define K_ERR_NET_CONN_RESET    ((kstatus_t)-1795)
#define K_ERR_NET_REFUSED       ((kstatus_t)-1796)
#define K_ERR_NET_UNREACHABLE   ((kstatus_t)-1797)

/* ── Power / Thermal ──────────── -2048 .. -2303 */
#define K_ERR_PWR_SUSPENDED     ((kstatus_t)-2048)
#define K_ERR_PWR_THROTTLED     ((kstatus_t)-2049)
#define K_ERR_PWR_THERMAL_TRIP  ((kstatus_t)-2050)
#define K_ERR_PWR_BATT_CRITICAL ((kstatus_t)-2051)
#define K_ERR_PWR_TRANS_DENIED  ((kstatus_t)-2052)

/* ── Policy / Profile / AI ────── -2304 .. -2559 */
#define K_ERR_POLICY_DENIED     ((kstatus_t)-2304)
#define K_ERR_PROFILE_RESTRICTED ((kstatus_t)-2305)
#define K_ERR_FEATURE_DISABLED  ((kstatus_t)-2306)
#define K_ERR_AI_DEFERRED       ((kstatus_t)-2307)
#define K_ERR_AI_THROTTLED      ((kstatus_t)-2308)

/*
 * Canonical kernel-internal -> syscall boundary status translation.
 *
 * NOTE: Kernel code must never return raw `sys_errno_t` or negative `sys_errno_t`
 * directly from internal functions. Always return `kstatus_t`. Translation must
 * occur exclusively at the syscall boundary.
 *
 * See `docs/architecture/kernel/status-code-contract.md` for full mapping rules.
 *
 * kstatus_to_sys_errno(): maps a kstatus_t into a stable positive SYS_E* code.
 * kstatus_to_sysret(): maps a kstatus_t into canonical syscall return encoding:
 *   K_OK -> 0, error -> -(sys_errno_t)
 */
sys_errno_t kstatus_to_sys_errno(kstatus_t st);
long kstatus_to_sysret(kstatus_t st);

#endif // BHARAT_KERNEL_STATUS_H
