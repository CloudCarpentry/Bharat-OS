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
    if target in ("stm32f4-renode", "stm32f4_discovery", "renode-stm32f4"):
        return "delivery/targets/renode/stm32f4_discovery.yaml"
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
    parser.add_argument("target", nargs="?", help="Target name (e.g., desktop-x86_64, stm32f4-renode). Optional if --target-yaml is provided.")
    parser.add_argument("--target-yaml", dest="target_yaml", help="Explicit YAML file for the build target.")
    parser.add_argument("--mode", choices=["development", "demo", "release"], help="Build mode")
    parser.add_argument("--smoke", action="store_true", help="Build, package, and execute smoke test verification")
    parser.add_argument("--interactive", action="store_true", help="Build, package, and launch interactive run session")
    parser.add_argument("--build-only", action="store_true", help="Compile only without running")
    parser.add_argument("--gui", action="store_true", help="Force GUI display")
    parser.add_argument("--headless", action="store_true", help="Force headless display")
    parsed_args, unknown = parser.parse_known_args(args)

    # If --smoke or --interactive is passed without --build-only, dispatch 'all' action to build.py
    if (parsed_args.smoke or parsed_args.interactive) and not parsed_args.build_only:
        action = "all"
    else:
        action = "build"

    cmd = [sys.executable, "tools/build.py", action]
    if parsed_args.target_yaml:
        target_yaml = parsed_args.target_yaml
    else:
        if not parsed_args.target:
            parser.error("Either <target> positional argument or --target-yaml must be supplied")
        target_yaml = map_target(parsed_args.target)
    cmd.extend(["--target-yaml", target_yaml])
    if parsed_args.mode:
        cmd.extend(["--mode", parsed_args.mode])
    if parsed_args.smoke and action == "all":
        cmd.append("--smoke")
    if parsed_args.interactive and action == "all":
        cmd.append("--interactive")
    if parsed_args.gui:
        cmd.append("--gui")
    if parsed_args.headless:
        cmd.append("--headless")
    cmd.extend(unknown)
    result = subprocess.run(cmd)
    return result.returncode

