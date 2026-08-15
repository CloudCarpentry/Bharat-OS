import sys
import re
from pathlib import Path

# Paths to scan
PATHS = ["core/services/power_mode", "core/services/process_manager", "core/services/vm_manager", "core/services/namesvc", "core/kernel/src/trap", "core/kernel/src/urpc"]

ALLOWLIST = [
    "sched_stub.c", "ds/cuckoo_hash.c", "ds/urpc_ring.c", "ds/radix_tree.c",
    "tests/", "boot_video_map.c", "fb_core.c", "irq_domain.c", "main.c",
    "boot_video.c", "device_mmio.c", "device_dma.c", "boot_display_bridge.c",
    "display_handoff.c", "boot_gui.c", "display_fallback.c", "sched_rt.c",
    "sched_class.c", "sched_migrate.c", "sched.c", "sched_wait.c", "sched_pi.c",
    "sched_deg.c", "sched_runqueue.c", "sched_topology.c", "trap.c",
    "init_bootstrap.c", "boot_mode.c", "sys_constraints.c", "linux_trap.c",
    "credentials.c", "isolation.c", "secure_boot.c", "mk_dispatch.c", "mk_proto.c",
    "kernel_boot.c", "tlb_shootdown.c", "tlb_flush.c", "tlb_pending.c", "tlb_asid.c",
    "iommu.c", "iommu_domain.c", "iommu_device.c", "dma.c",
    "numa_policy.c", "pmm.c", "numa.c", "footprint.c", "zswap.c",
    "vmm.c", "vm_mapping.c", "vm_space.c", "vm_object.c", "freestanding_string.c"
]

def is_allowlisted(file_path):
    path_str = str(file_path.as_posix())
    for allow in ALLOWLIST:
        if allow in path_str:
            return True
    return False

def check_file(file_path):
    if is_allowlisted(file_path):
        return 0

    errors = 0
    with open(file_path, "r", encoding="utf-8") as f:
        lines = f.readlines()

    for i, line in enumerate(lines):
        if re.search(r'\breturn\s+-1\s*;', line):
            print(f"{file_path}:{i+1}: Raw 'return -1;' found: {line.strip()}")
            errors += 1

    return errors

def main():
    repo_root = Path(__file__).parent.parent.parent
    errors = 0

    for p in PATHS:
        target_dir = repo_root / p
        for child in target_dir.rglob("*.c"):
            errors += check_file(child)

    if errors > 0:
        print(f"FAILED: Found {errors} raw status code violations.")
        sys.exit(1)
    else:
        print("SUCCESS: No raw status code violations found in strict service APIs.")
        sys.exit(0)

if __name__ == '__main__':
    main()
