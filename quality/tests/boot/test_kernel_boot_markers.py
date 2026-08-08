from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]
KERNEL_MARKER_SOURCES = [
    REPO_ROOT / "core/kernel/src/init/init_bootstrap.c",
    REPO_ROOT / "core/kernel/src/kernel_boot.c",
]


def test_kernel_does_not_synthesize_userspace_success_markers():
    forbidden = ["USER_INIT:", "BOOT_RUNTIME: STABLE"]
    offenders = []
    for path in KERNEL_MARKER_SOURCES:
        text = path.read_text()
        for marker in forbidden:
            if marker in text:
                offenders.append(f"{path.relative_to(REPO_ROOT)} contains {marker}")
    assert offenders == []


def test_missing_init_module_is_reported_by_loader_not_isa_gate():
    bootstrap = (REPO_ROOT / "core/kernel/src/init/init_bootstrap.c").read_text()
    kernel_boot = (REPO_ROOT / "core/kernel/src/kernel_boot.c").read_text()
    assert "BOOT_FAIL: INIT_MODULE_MISSING" in bootstrap
    assert "BOOT_FAIL: INIT_MODULE_MISSING" not in kernel_boot


def test_kernel_readiness_is_not_boot_completion():
    kernel_boot = (REPO_ROOT / "core/kernel/src/kernel_boot.c").read_text()
    assert "KERNEL_RUNTIME_READY" in kernel_boot
    assert "USERSPACE_LAUNCH_BEGIN" in kernel_boot
    assert "Kernel loading successful - boot complete" not in kernel_boot
