with open("kernel/src/mm/tlb/tlb_shootdown.c", "r") as f:
    c = f.read()

# We completely removed `tlb_collect_targets` but left the fallback reference here in vmm_send_tlb_invalidate
# If `!aspace`, we shouldn't reach this because we already have `if (!aspace) return;` at the top of `vmm_send_tlb_invalidate`!
# Ah wait, I missed removing the block entirely.
c = c.replace("""    uint64_t target_mask = 0;
    if (as) {
        // We still increment the global aspace sequence, but we use the pending table's reqid for URPC tracking.
        req.generation = __atomic_add_fetch(&as->tlb_gen, 1, __ATOMIC_SEQ_CST);
        target_mask = __atomic_load_n(&as->active_mask, __ATOMIC_ACQUIRE);
    } else {
        req.generation = 1;
        target_mask = tlb_collect_targets(aspace ? aspace->object_id : 0); // Fallback to basic loop scan
    }""",
"""    uint64_t target_mask = __atomic_load_n(&as->active_mask, __ATOMIC_ACQUIRE);
    req.generation = __atomic_add_fetch(&as->tlb_gen, 1, __ATOMIC_SEQ_CST);""")

with open("kernel/src/mm/tlb/tlb_shootdown.c", "w") as f:
    f.write(c)
