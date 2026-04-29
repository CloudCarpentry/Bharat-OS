# SDK Syscall Wrapper Guide

## Status
Accepted

## Scope
How to use and implement SDK syscall wrappers.

## Contract
The SDK provides a single symbol `bharat_syscall`:
```c
long bharat_syscall(long sysno, long arg1, ..., long arg6);
```

## Per-Architecture Behavior
Implementation resides in `experience/user/sdk/lib/syscall/syscall_[arch].S`.
- Ensure alignment with kernel `syscall_extract.c`.
