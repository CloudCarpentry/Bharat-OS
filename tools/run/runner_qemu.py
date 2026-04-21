import json
import os
import subprocess
import sys
from pathlib import Path
from shutil import which


def check_command(cmd_name: str) -> bool:
    return which(cmd_name) is not None


def load_run_manifest(path: Path) -> dict:
    if not path.exists():
        raise FileNotFoundError(f"Run manifest not found: {path}")
    with open(path, "r") as f:
        return json.load(f)


def run_qemu(manifest_path: Path) -> int:
    manifest = load_run_manifest(manifest_path)

    target_name = manifest.get("target_name")
    print(f"\n[Run] Launching QEMU for {target_name}...")

    arch = manifest.get("arch")
    run_config = manifest.get("run_config", {})
    artifacts = manifest.get("artifacts", {})
    boot_contract = manifest.get("boot_contract", {})

    boot_artifact = artifacts.get("boot_artifact")

    if not boot_artifact or not os.path.exists(boot_artifact):
        print(f"Error: Boot artifact not found: {boot_artifact}")
        sys.exit(1)

    print(f"[Run] Using boot artifact: {boot_artifact}")

    qemu_bin_map = {
        "x86_64": "qemu-system-x86_64",
        "arm64": "qemu-system-aarch64",
        "riscv64": "qemu-system-riscv64",
        "arm32": "qemu-system-arm",
        "riscv32": "qemu-system-riscv32",
    }

    runner = qemu_bin_map.get(arch)
    if not runner:
        print(f"Error: Unknown architecture '{arch}' for QEMU runner.")
        sys.exit(1)

    if not check_command(runner):
        print(f"\nERROR: Runner '{runner}' not found in PATH.")
        sys.exit(1)

    cmd = [runner]

    machine = run_config.get("machine")
    if machine:
        cmd.extend(["-machine", machine])

    cpu = run_config.get("cpu")
    if cpu:
        cmd.extend(["-cpu", cpu])

    memory = run_config.get("memory")
    if memory:
        cmd.extend(["-m", memory])

    protocol = boot_contract.get("protocol")
    dtb_info = boot_contract.get("dtb", {})
    dtb_path = dtb_info.get("path")

    if dtb_info.get("required") and not dtb_path:
        print("Error: QEMU Runner requires DTB path but none was provided.")
        sys.exit(1)

    # Basic generic boot logic based on protocol
    cmd.extend(["-kernel", boot_artifact])

    if dtb_path:
        cmd.extend(["-dtb", dtb_path])

    # We use discrete flags for better signal handling on Windows/WSL
    # -nographic often hijacks the signal handler
    cmd.extend(["-nographic", "-monitor", "none", "-serial", "stdio"])

    smp = run_config.get("smp", 1)
    if smp:
        cmd.extend(["-smp", str(smp)])

    extra_args = run_config.get("extra_args", [])
    if extra_args:
        cmd.extend(extra_args)

    print(f"[Run] Executing: {' '.join(cmd)}")

    import time

    proc = None
    try:
        proc = subprocess.Popen(cmd)
        # Polling loop allows KeyboardInterrupt (Ctrl+C) to be caught on Windows
        while proc.poll() is None:
            time.sleep(0.1)
    except KeyboardInterrupt:
        print("\n[Run] Terminating QEMU...")
        if proc:
            # Try gentle termination first
            proc.terminate()
            try:
                proc.wait(timeout=2)
            except subprocess.TimeoutExpired:
                proc.kill()
        return 0

    if proc and proc.returncode != 0:
        print(f"[Run] QEMU exited with code {proc.returncode}")
        return proc.returncode

    return 0
