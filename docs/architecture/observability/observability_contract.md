<!-- SPDX-License-Identifier: MIT -->
# Observability contract v1

Diagnostics are optional mechanisms, never a correctness dependency. The pointer-free UAPI header is 40 bytes and carries explicit ABI/header/payload sizes, severity, source kind, source/CPU/subsystem identifiers, sequence, and monotonic nanoseconds. Unknown event types are retained by compatible readers; unknown ABI versions fail closed.

Each caller-owned ring has exactly one writer and one reader. The writer owns `write_position`, the reader owns `read_position`, and release/acquire operations publish and observe completed slots. No global state, lock, allocation, blocking, retry loop, remote mutation, or backend dependency exists. A full ring drops the newest event and increments `dropped`, preserving unread evidence. Reset is legal only while both owners are quiescent.

Emission rejects malformed metadata and over-sized payloads. A nonzero payload is a trusted in-domain buffer supplied by the caller; the API cannot prove an arbitrary C pointer valid. Fatal severity is descriptive and has no panic side effect. Disabled emission returns `BH_ERR_NOT_SUPPORTED` without accessing its arguments.

## Security and public evidence

Public bug reports may contain stage identifiers, status/failure categories, redacted source IDs, counts, and bounded non-sensitive messages. They must omit capability tokens, credentials, keys, user memory, kernel pointers, authentication material, full environments, and sensitive device identifiers. Address-like fields are omitted unless a privileged evidence policy explicitly redacts and authorizes them.
