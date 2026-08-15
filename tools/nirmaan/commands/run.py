import argparse
import subprocess
import sys

def map_target(target):
    if target == "desktop-x86_64":
        return "delivery/targets/qemu/x86_64_desktop_gui.yaml"
    if target == "desktop-arm64":
        return "delivery/targets/qemu/arm64_desktop_gui.yaml"
    if target == "desktop-riscv64":
        return "delivery/targets/qemu/riscv64_desktop_gui.yaml"
    if target == "controller-arm32":
        return "delivery/targets/qemu/arm32_mmu_lite_headless.yaml" # mmu lite doesn't have gui

    target = target.replace("-", "_")
    if not target.endswith(".yaml") and not target.endswith("_headless") and not target.endswith("_gui"):
         return f"delivery/targets/qemu/{target}_gui.yaml"
    return target

def run(args):
    parser = argparse.ArgumentParser(description="Run a target")
    parser.add_argument("target", help="The target to run")

    parsed_args, unknown = parser.parse_known_args(args)

    cmd = [sys.executable, "tools/build.py", "run"]

    target_yaml = map_target(parsed_args.target)

    cmd.extend(["--target-yaml", target_yaml])

    cmd.extend(unknown)

    result = subprocess.run(cmd)
    return result.returncode
