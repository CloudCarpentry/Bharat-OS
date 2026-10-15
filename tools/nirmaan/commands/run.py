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
    if target in ("stm32f4-renode", "stm32f4_discovery", "renode-stm32f4"):
        return "delivery/targets/renode/stm32f4_discovery.yaml"
    if target == "x86_64_hmem_demo":
        return "delivery/targets/qemu/x86_64_hmem_demo.yaml"
    if target == "arm64_hmem_demo":
        return "delivery/targets/qemu/arm64_hmem_demo.yaml"
    if target == "riscv64_hmem_demo":
        return "delivery/targets/qemu/riscv64_hmem_demo.yaml"

    target = target.replace("-", "_")
    if not target.endswith(".yaml") and not target.endswith("_headless") and not target.endswith("_gui"):
         return f"delivery/targets/qemu/{target}_gui.yaml"
    return target

def run(args):
    parser = argparse.ArgumentParser(description="Run a target")
    parser.add_argument("target", nargs="?", help="The target to run (e.g., desktop-x86_64, stm32f4-renode). Optional if --target-yaml is provided.")
    parser.add_argument("--target-yaml", dest="target_yaml", help="Explicit YAML file for the run target.")
    parser.add_argument("--smoke", action="store_true", help="Run in headless smoke verification mode")
    parser.add_argument("--interactive", action="store_true", help="Run in interactive session mode")
    parser.add_argument("--headless", action="store_true", help="Force headless display")
    parser.add_argument("--gui", action="store_true", help="Force GUI display")

    parsed_args, unknown = parser.parse_known_args(args)

    cmd = [sys.executable, "tools/build.py", "run"]

    if parsed_args.target_yaml:
        target_yaml = parsed_args.target_yaml
    else:
        if not parsed_args.target:
            parser.error("Either <target> positional argument or --target-yaml must be supplied")
        target_yaml = map_target(parsed_args.target)

    cmd.extend(["--target-yaml", target_yaml])
    if parsed_args.smoke:
        cmd.append("--smoke")
    if parsed_args.interactive:
        cmd.append("--interactive")
    if parsed_args.headless:
        cmd.append("--headless")
    if parsed_args.gui:
        cmd.append("--gui")

    cmd.extend(unknown)

    result = subprocess.run(cmd)
    return result.returncode

