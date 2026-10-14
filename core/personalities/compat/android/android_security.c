#include "compat/android/android_security.h"
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Distributed Mediation Hook: Binder Transaction Check
 *
 * Implements fail-closed capability-backed SELinux policy checks.
 * Denies invalid SIDs, zero actions, and unpermitted cross-domain invocations.
 */
int android_sec_check_binder_access(android_sec_sid_t caller_sid, android_sec_sid_t target_sid, uint32_t action) {
    /* Fail closed on invalid SIDs or invalid action */
    if (caller_sid == ANDROID_SEC_SID_INVALID || target_sid == ANDROID_SEC_SID_INVALID) {
        return -13; /* -EACCES */
    }
    if (action == 0) {
        return -22; /* -EINVAL */
    }

    /* Intra-domain transactions are always permitted */
    if (caller_sid == target_sid) {
        return 0;
    }

    /* Privileged domains (Kernel and Init) have universal authority */
    if (caller_sid == ANDROID_SEC_SID_KERNEL || caller_sid == ANDROID_SEC_SID_INIT) {
        return 0;
    }

    /* ServiceManager resolution policy */
    if (target_sid == ANDROID_SEC_SID_SERVICEMANAGER) {
        /* Isolated apps are strictly prohibited from querying ServiceManager directly */
        if (caller_sid == ANDROID_SEC_SID_ISOLATED_APP) {
            return -13; /* -EACCES */
        }
        /* Only privileged callers can impersonate across ServiceManager */
        if (action == ANDROID_BINDER_ACTION_IMPERSONATE) {
            return -1; /* -EPERM */
        }
        return 0;
    }

    /* System server policy: allowed to transact with apps and zygote */
    if (caller_sid == ANDROID_SEC_SID_SYSTEM_SERVER) {
        if (target_sid == ANDROID_SEC_SID_APP ||
            target_sid == ANDROID_SEC_SID_ZYGOTE ||
            target_sid == ANDROID_SEC_SID_ISOLATED_APP) {
            return 0;
        }
    }

    /* Standard application domain policy: may call system server and service manager */
    if (caller_sid == ANDROID_SEC_SID_APP) {
        if (target_sid == ANDROID_SEC_SID_SYSTEM_SERVER) {
            return 0;
        }
        /* Apps cannot transact directly with Zygote, Init, or Kernel */
        return -13; /* -EACCES */
    }

    /* Isolated app domain policy: isolated sandboxes cannot originate arbitrary binder calls */
    if (caller_sid == ANDROID_SEC_SID_ISOLATED_APP) {
        return -13; /* -EACCES */
    }

    /* Fail closed for all unmapped security combinations */
    return -13; /* -EACCES */
}

/**
 * @brief Distributed Mediation Hook: Shared Memory Access Check
 *
 * Verifies object capabilities against requested memory protections.
 */
int android_sec_check_memory_access(android_sec_sid_t process_sid, android_logical_obj_t* obj, int prot) {
    if (process_sid == ANDROID_SEC_SID_INVALID || obj == NULL || prot < 0) {
        return -13; /* -EACCES */
    }

    /* Querying access without protection flags (prot == 0) succeeds if object is valid */
    if (prot == 0) {
        return 0;
    }

    /* Isolated applications are prohibited from obtaining write access to shared memory */
    if (process_sid == ANDROID_SEC_SID_ISOLATED_APP && (prot & 2 /* PROT_WRITE */)) {
        return -13; /* -EACCES */
    }

    /* Check backing capability if initialized */
    if (obj->backing_cap.generation != 0) {
        /* If write requested, ensure write right is present */
        if ((prot & 2) && !(obj->backing_cap.slot & 0x2)) {
            return -13; /* -EACCES */
        }
    }

    return 0;
}

/**
 * @brief Transitions a process into a new security domain.
 *
 * Follows Android process spawn lifecycle:
 * - KERNEL -> INIT
 * - INIT -> SERVICEMANAGER / SYSTEM_SERVER / ZYGOTE
 * - ZYGOTE -> APP / ISOLATED_APP
 * - APP -> ISOLATED_APP
 */
int android_sec_transition_domain(android_sec_sid_t current_sid, android_sec_sid_t new_sid) {
    if (current_sid == ANDROID_SEC_SID_INVALID || new_sid == ANDROID_SEC_SID_INVALID) {
        return -22; /* -EINVAL */
    }

    /* Staying in the same domain is always valid */
    if (current_sid == new_sid) {
        return 0;
    }

    /* Kernel bootstrap */
    if (current_sid == ANDROID_SEC_SID_KERNEL && new_sid == ANDROID_SEC_SID_INIT) {
        return 0;
    }

    /* Init daemon spawning */
    if (current_sid == ANDROID_SEC_SID_INIT) {
        if (new_sid == ANDROID_SEC_SID_SERVICEMANAGER ||
            new_sid == ANDROID_SEC_SID_SYSTEM_SERVER ||
            new_sid == ANDROID_SEC_SID_ZYGOTE) {
            return 0;
        }
    }

    /* Zygote application spawning */
    if (current_sid == ANDROID_SEC_SID_ZYGOTE) {
        if (new_sid == ANDROID_SEC_SID_APP ||
            new_sid == ANDROID_SEC_SID_ISOLATED_APP) {
            return 0;
        }
    }

    /* App spawning an isolated sandbox process */
    if (current_sid == ANDROID_SEC_SID_APP && new_sid == ANDROID_SEC_SID_ISOLATED_APP) {
        return 0;
    }

    /* All other domain transitions (e.g. privilege escalation) are strictly denied */
    return -1; /* -EPERM */
}
