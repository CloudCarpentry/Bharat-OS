import argparse
import subprocess
import sys

def map_target(target):
    # Map known high‑level targets to their concrete YAML files
    if target == "desktop-x86_64":
        return "delivery/targets/qemu/x86_64_desktop_headless.yaml"
    if target == "desktop-arm64":
        return "delivery/targets/qemu/arm64_desktop_headless.yaml"
    if target == "desktop-riscv64":
        return "delivery/targets/qemu/riscv64_desktop_headless.yaml"
    if target == "controller-arm32":
        return "delivery/targets/qemu/arm32_mmu_lite_headless.yaml"
    # Explicit HMEM demo targets
    if target == "x86_64_hmem_demo":
        return "delivery/targets/qemu/x86_64_hmem_demo.yaml"
    if target == "arm64_hmem_demo":
        return "delivery/targets/qemu/arm64_hmem_demo.yaml"
    if target == "riscv64_hmem_demo":
        return "delivery/targets/qemu/riscv64_hmem_demo.yaml"
    # Generic fallback – convert dashes to underscores and add _headless.yaml
    target = target.replace("-", "_")
    if not target.endswith(".yaml") and not target.endswith("_headless") and not target.endswith("_gui"):
        return f"delivery/targets/qemu/{target}_headless.yaml"
    return target

def run(args):
    parser = argparse.ArgumentParser(description="Build a target")
    parser.add_argument("target", nargs="?", help="Target name (e.g., desktop-x86_64). Optional if --target-yaml is provided.")
    parser.add_argument("--target-yaml", dest="target_yaml", help="Explicit YAML file for the build target.")
    parser.add_argument("--mode", choices=["development", "demo", "release"], help="Build mode")
    parsed_args, unknown = parser.parse_known_args(args)
    cmd = [sys.executable, "tools/build.py", "build"]
    if parsed_args.target_yaml:
        target_yaml = parsed_args.target_yaml
    else:
        if not parsed_args.target:
            parser.error("Either <target> positional argument or --target-yaml must be supplied")
        target_yaml = map_target(parsed_args.target)
    cmd.extend(["--target-yaml", target_yaml])
    if parsed_args.mode:
        cmd.extend(["--mode", parsed_args.mode])
    cmd.extend(unknown)
    result = subprocess.run(cmd)
    return result.returncode
