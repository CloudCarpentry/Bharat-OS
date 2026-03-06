#ifndef BHARAT_POSIX_UNISTD_H
#define BHARAT_POSIX_UNISTD_H

/*
 * POSIX API Compatibility Layer
 * This provides the native POSIX interface for Bharat-OS user-space.
 * Calls made here translate natively into Microkernel IPC messages.
 */

#include <stddef.h>
#include <stdint.h>

/* Use conditional guards to prevent redefinition errors
 * when building with standard toolchains.
 */

#ifndef _SSIZE_T_DEFINED_
#define _SSIZE_T_DEFINED_
typedef intptr_t ssize_t;
#endif

#ifndef _PID_T_DEFINED_
#define _PID_T_DEFINED_
typedef int32_t pid_t;
#endif

#ifndef _OFF_T_DEFINED_
#define _OFF_T_DEFINED_
typedef int64_t off_t;
#endif

// Standard File Descriptors
#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

// Basic POSIX IO
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
int close(int fd);

// Process Control
void _exit(int status);
int fork(void);
int execve(const char *pathname, char *const argv[], char *const envp[]);
pid_t getpid(void);

// Syscall wrapper (translates POSIX to Microkernel IPC)
long syscall(long number, ...);

#endif // BHARAT_POSIX_UNISTD_H
