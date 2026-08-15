import argparse
import subprocess
import sys

def map_target(target):
    target = target.replace("-", "_")
    if not target.endswith(".yaml") and not target.endswith("_headless") and not target.endswith("_gui"):
         return f"delivery/targets/qemu/{target}_headless.yaml"
    return target

def run(args):
    parser = argparse.ArgumentParser(description="Debug a target")
    parser.add_argument("target", help="The target to debug")

    parsed_args, unknown = parser.parse_known_args(args)

    cmd = [sys.executable, "tools/build.py", "debug"]

    target_yaml = map_target(parsed_args.target)

    cmd.extend(["--target-yaml", target_yaml])

    cmd.extend(unknown)

    result = subprocess.run(cmd)
    return result.returncode
