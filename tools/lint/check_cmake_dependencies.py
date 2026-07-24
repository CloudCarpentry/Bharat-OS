#!/usr/bin/env python3
"""Bharat-OS CMake Target-Dependency Linter.

Uses the CMake File API to extract target dependency structures
and enforce architecture-boundary policies.
"""

import os
import sys
import json
import glob
import argparse
import subprocess

FORBIDDEN_RULES = [
    # (source_layer, target_layer, error_msg)
    ("kernel", "services", "Kernel core must never depend upward on services"),
    ("kernel", "personalities", "Kernel core must never depend on personalities"),
    ("hal_arch", "services", "HAL/Arch/Platform must never depend on services"),
    ("interface", "kernel", "Interface/UAPI contracts must never depend on kernel implementation"),
]

def register_query(build_dir):
    query_dir = os.path.join(build_dir, ".cmake", "api", "v1", "query", "client-linter")
    os.makedirs(query_dir, exist_ok=True)
    query_file = os.path.join(query_dir, "query.json")
    query_data = {
        "requests": [
            { "kind": "codemodel", "version": 2 }
        ]
    }
    with open(query_file, "w") as f:
        json.dump(query_data, f, indent=2)

def run_configure(preset):
    print(f"Configuring preset '{preset}' to update CMake File API codemodel...")
    res = subprocess.run(["cmake", "--preset", preset], capture_output=True, text=True)
    if res.returncode != 0:
        print(f"Error during CMake configure:\n{res.stderr}")
        return False
    return True

def get_layer(source_dir):
    if not source_dir:
        return "other"
    source_dir = source_dir.replace("\\", "/").strip("/")
    if source_dir.startswith("core/kernel"):
        return "kernel"
    if source_dir.startswith("core/hal") or source_dir.startswith("core/arch") or source_dir.startswith("core/platform"):
        return "hal_arch"
    if source_dir.startswith("core/services"):
        return "services"
    if source_dir.startswith("core/personalities"):
        return "personalities"
    if source_dir.startswith("interface"):
        return "interface"
    return "other"

def analyze_dependencies(build_dir, baseline=None):
    reply_dir = os.path.join(build_dir, ".cmake", "api", "v1", "reply")
    codemodels = glob.glob(os.path.join(reply_dir, "codemodel-v2-*.json"))
    if not codemodels:
        print("Error: No codemodel-v2 reply found. Make sure CMake configure succeeded.")
        return None

    # Load the latest reply
    codemodel_path = sorted(codemodels)[-1]
    with open(codemodel_path, "r") as f:
        data = json.load(f)

    targets_by_name = {}

    # Resolve all target files
    configurations = data.get("configurations", [])
    if not configurations:
        return None

    targets = configurations[0].get("targets", [])

    # Read each target JSON file to classify it and get its dependencies
    for t_ref in targets:
        target_json_name = t_ref.get("jsonFile")
        if not target_json_name:
            continue
        target_path = os.path.join(reply_dir, target_json_name)
        if not os.path.exists(target_path):
            continue
        with open(target_path, "r") as f:
            t_data = json.load(f)
            name = t_data.get("name")
            paths = t_data.get("paths", {})
            source_dir = paths.get("source", "")
            layer = get_layer(source_dir)

            # Extract dependencies
            deps = []
            for dep in t_data.get("dependencies", []):
                dep_id = dep.get("id", "")
                # Extract target name from ID (e.g. "subsys_manager::@abc")
                dep_name = dep_id.split("::")[0]
                deps.append(dep_name)

            targets_by_name[name] = {
                "name": name,
                "layer": layer,
                "source_dir": source_dir,
                "dependencies": deps
            }

    violations = []

    # Enforce rules
    for name, info in targets_by_name.items():
        src_layer = info["layer"]
        for dep_name in info["dependencies"]:
            dep_info = targets_by_name.get(dep_name)
            if not dep_info:
                continue
            dst_layer = dep_info["layer"]

            # Check rules
            for rule_src, rule_dst, err_msg in FORBIDDEN_RULES:
                if src_layer == rule_src and dst_layer == rule_dst:
                    violation_key = f"{name} -> {dep_name}"
                    if baseline and violation_key in baseline:
                        continue
                    violations.append({
                        "source_target": name,
                        "source_layer": src_layer,
                        "target_target": dep_name,
                        "target_layer": dst_layer,
                        "message": err_msg
                    })

    return violations

def main():
    parser = argparse.ArgumentParser(description="CMake Target-Dependency Linter")
    parser.add_argument("--preset", default="x86_64-dev", help="CMake configure preset to analyze")
    parser.add_argument("--build-dir", default="build/x86_64-dev", help="Path to build directory")
    parser.add_argument("--baseline", help="JSON file with allowed baseline exceptions")
    parser.add_argument("--report", help="Output markdown report path")
    parser.add_argument("--strict", action="store_true", help="Fail with non-zero on violations")

    args = parser.parse_args()

    # Register the File API query
    register_query(args.build_dir)

    # Run CMake configure to populate the API files
    if not run_configure(args.preset):
        sys.exit(1)

    # Load baseline
    baseline_set = set()
    if args.baseline and os.path.exists(args.baseline):
        with open(args.baseline, "r") as f:
            baseline_set = set(json.load(f))

    # Analyze
    violations = analyze_dependencies(args.build_dir, baseline_set)
    if violations is None:
        print("Analysis failed.")
        sys.exit(1)

    print(f"\n--- CMake Dependency Linter Results ({args.preset}) ---")
    print(f"Violations found: {len(violations)}")
    for v in violations:
        print(f"  [VIOLATION] {v['source_target']} ({v['source_layer']}) -> {v['target_target']} ({v['target_layer']})")
        print(f"    Reason: {v['message']}")

    # Write report if requested
    if args.report:
        with open(args.report, "w") as f:
            f.write("# CMake Target-Dependency Linter Report\n\n")
            f.write(f"- Build Preset analyzed: **{args.preset}**\n")
            f.write(f"- Violations found: **{len(violations)}**\n\n")
            if violations:
                f.write("| Source Target | Source Layer | Dependency | Target Layer | Error Message |\n")
                f.write("|---|---|---|---|---|\n")
                for v in violations:
                    f.write(f"| `{v['source_target']}` | `{v['source_layer']}` | `{v['target_target']}` | `{v['target_layer']}` | {v['message']} |\n")
            else:
                f.write("No dependency violations detected. Architecture boundaries are clean!\n")
        print(f"Report written to {args.report}")

    if args.strict and violations:
        sys.exit(1)
    sys.exit(0)

if __name__ == "__main__":
    main()
