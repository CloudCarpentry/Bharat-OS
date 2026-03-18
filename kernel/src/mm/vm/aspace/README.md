# VM ASpace Layer
- Owns region reservations, overlap checks, attach/detach object mappings.
- May call: VM object APIs, HAL PT/TLB APIs.
- Forbidden: Fault layer directly.
