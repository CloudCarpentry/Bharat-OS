/* SPDX-License-Identifier: MIT */
#include "linux_procfs.h"
#include "linux_errno.h"
#include "syscall/usercopy.h"
#include "lib/base/string.h"

#define MAX_LINUX_PROCFS_FILES 64

static linux_procfs_file_t g_linux_procfs_pool[MAX_LINUX_PROCFS_FILES];

static linux_procfs_file_t *linux_procfs_alloc(void) {
    for (int i = 0; i < MAX_LINUX_PROCFS_FILES; i++) {
        if (!g_linux_procfs_pool[i].in_use) {
            memset(&g_linux_procfs_pool[i], 0, sizeof(linux_procfs_file_t));
            g_linux_procfs_pool[i].in_use = 1;
            g_linux_procfs_pool[i].ref_count = 1;
            return &g_linux_procfs_pool[i];
        }
    }
    return NULL;
}

void linux_procfs_file_ref(linux_procfs_file_t *pf) {
    if (pf) {
        pf->ref_count++;
    }
}

void linux_procfs_file_unref(linux_procfs_file_t *pf) {
    if (!pf) return;
    pf->ref_count--;
    if (pf->ref_count <= 0) {
        pf->in_use = 0;
        pf->size = 0;
    }
}

static size_t procfs_set_content(linux_procfs_file_t *pf, const char *src) {
    size_t len = strlen(src);
    if (len >= LINUX_PROCFS_MAX_CONTENT) {
        len = LINUX_PROCFS_MAX_CONTENT - 1;
    }
    memcpy(pf->data, src, len);
    pf->data[len] = '\0';
    pf->size = len;
    return len;
}

linux_procfs_file_t *linux_procfs_generate(const char *path, struct bh_process *proc) {
    if (!path) return NULL;

    linux_procfs_file_t *pf = linux_procfs_alloc();
    if (!pf) return NULL;

    (void)proc;

    if (strcmp(path, "/proc/self/status") == 0 || strcmp(path, "/proc/status") == 0) {
        procfs_set_content(pf,
            "Name:\tbharat_app\n"
            "State:\tR (running)\n"
            "Tgid:\t1\n"
            "Pid:\t1\n"
            "PPid:\t1\n"
            "VmSize:\t65536 kB\n"
            "VmRSS:\t16384 kB\n"
            "Threads:\t1\n");
    } else if (strcmp(path, "/proc/self/cmdline") == 0 || strcmp(path, "/proc/cmdline") == 0) {
        const char *cmd = "/bin/sh";
        size_t len = strlen(cmd) + 1;
        if (len < LINUX_PROCFS_MAX_CONTENT) {
            memcpy(pf->data, cmd, len);
            pf->size = len;
        }
    } else if (strcmp(path, "/proc/cpuinfo") == 0) {
        procfs_set_content(pf,
            "processor\t: 0\n"
            "vendor_id\t: BharatCPU\n"
            "cpu family\t: 6\n"
            "model name\t: Bharat-OS Verified Architecture\n"
            "cpu MHz\t\t: 2400.000\n"
            "bogomips\t: 4800.00\n"
            "flags\t\t: fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush\n\n");
    } else if (strcmp(path, "/proc/meminfo") == 0) {
        procfs_set_content(pf,
            "MemTotal:       2097152 kB\n"
            "MemFree:        1835008 kB\n"
            "MemAvailable:   1835008 kB\n"
            "Buffers:          32768 kB\n"
            "Cached:          131072 kB\n"
            "SwapTotal:             0 kB\n"
            "SwapFree:              0 kB\n");
    } else if (strcmp(path, "/proc/version") == 0) {
        procfs_set_content(pf,
            "Linux version 5.15.0-bharat (gcc 12.2.0) #1 SMP Bharat-OS 2026\n");
    } else if (strcmp(path, "/proc/mounts") == 0 || strcmp(path, "/proc/self/mounts") == 0) {
        procfs_set_content(pf,
            "rootfs / rootfs rw,relatime 0 0\n"
            "proc /proc proc rw,nosuid,nodev,noexec,relatime 0 0\n"
            "devpts /dev/pts devpts rw,nosuid,noexec,relatime 0 0\n"
            "tmpfs /tmp tmpfs rw,nosuid,nodev,relatime 0 0\n");
    } else if (strcmp(path, "/proc/self/maps") == 0 || strcmp(path, "/proc/maps") == 0) {
        procfs_set_content(pf,
            "00400000-00452000 r-xp 00000000 00:00 0 [text]\n"
            "00452000-00453000 r--p 00052000 00:00 0 [rodata]\n"
            "00453000-00454000 rw-p 00053000 00:00 0 [data]\n"
            "7fff0000-7fff2000 rw-p 00000000 00:00 0 [stack]\n");
    } else {
        linux_procfs_file_unref(pf);
        return NULL;
    }

    return pf;
}

long linux_procfs_read(linux_procfs_file_t *pf, uint64_t *offset, void *buf, size_t count) {
    if (!pf || !offset || !buf) return -LINUX_EFAULT;
    if (count == 0) return 0;

    if (*offset >= pf->size) {
        return 0; // EOF
    }

    size_t avail = pf->size - *offset;
    size_t chunk = count;
    if (chunk > avail) {
        chunk = avail;
    }

    bh_status_t s = bh_copy_to_user(buf, pf->data + *offset, chunk);
    if (s != BH_OK) {
        return -LINUX_EFAULT;
    }

    *offset += chunk;
    return (long)chunk;
}
