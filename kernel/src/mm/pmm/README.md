# PMM Layer
- Owns physical frame discovery, zones, buddy/contiguous alloc, refcounts, pinned/reserved classes.
- May call: None
- Forbidden: VM policy, fault layers, arch internals.
