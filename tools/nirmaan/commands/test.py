import argparse
import subprocess
import sys

def map_target(target):
    if target == "desktop-x86_64":
        return "delivery/targets/qemu/x86_64_desktop_headless.yaml"
    if target == "desktop-arm64":
        return "delivery/targets/qemu/arm64_desktop_headless.yaml"
    if target == "desktop-riscv64":
        return "delivery/targets/qemu/riscv64_desktop_headless.yaml"
    if target == "controller-arm32":
        return "delivery/targets/qemu/arm32_mmu_lite_headless.yaml"
    if target in ("stm32f4-renode", "stm32f4_discovery", "renode-stm32f4"):
        return "delivery/targets/renode/stm32f4_discovery.yaml"
    target = target.replace("-", "_")
    if not target.endswith(".yaml") and not target.endswith("_headless") and not target.endswith("_gui"):
         return f"delivery/targets/qemu/{target}_headless.yaml"
    return target

def run(args):
    parser = argparse.ArgumentParser(description="Test a target or subsystem")
    parser.add_argument("target", help="The target or subsystem to test (e.g., desktop-x86_64, board, stm32f4-renode)")

    parsed_args, unknown = parser.parse_known_args(args)

    if parsed_args.target in ("board", "boards", "bsp"):
        # Run board unit tests and validation
        print("[Nirmaan] Running Embedded Board Qualification & Unit Tests...")
        val_res = subprocess.run([sys.executable, "tools/board/validate.py", "--all"])
        if val_res.returncode != 0:
            return val_res.returncode
        return subprocess.run([sys.executable, "-m", "pytest", "quality/tests/board/"]).returncode

    cmd = [sys.executable, "tools/build.py", "all"]

    target_yaml = map_target(parsed_args.target)

    cmd.extend(["--target-yaml", target_yaml, "--smoke"])

    cmd.extend(unknown)

    result = subprocess.run(cmd)
    return result.returncode
