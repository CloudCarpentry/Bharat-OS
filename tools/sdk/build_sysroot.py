#!/usr/bin/env python3
import os
import argparse
import subprocess
import json
from pathlib import Path

def build_sysroot():
    parser = argparse.ArgumentParser(description="Bharat-OS SDK Sysroot Builder")
    parser.add_argument("--arch", required=True, help="Target architecture (e.g., x86_64, arm64, riscv64)")
    parser.add_argument("--profile", required=True, help="Target device profile (e.g., desktop, appliance)")
    parser.add_argument("--build-type", default="Release", help="CMake build type (Debug/Release)")
    parser.add_argument("--out-dir", required=True, help="Output directory for the sysroot")
    parser.add_argument("--dry-run", action="store_true", help="Print commands without executing them")

    args = parser.parse_args()

    out_dir = Path(args.out_dir).absolute()
    build_dir = Path("build") / f"sdk-sysroot-{args.arch}-{args.profile}-{args.build_type.lower()}"

    # We do not blindly remove user-supplied out_dir. We simply install into it.

    preset_name = f"{args.arch}-qemu-{args.profile}-{args.build_type.lower()}"

    # We use CMake generators instead of make directly
    cmake_config_cmd = [
        "cmake",
        "--preset", preset_name,
        "-S", ".",
        "-B", str(build_dir),
        f"-DCMAKE_INSTALL_PREFIX={out_dir}"
    ]

    cmake_build_cmd = [
        "cmake", "--build", str(build_dir)
    ]

    cmake_install_cmd = [
        "cmake", "--install", str(build_dir)
    ]

    print("Configuring sysroot build...")
    print(" ".join(cmake_config_cmd))
    if not args.dry_run:
        os.makedirs(build_dir, exist_ok=True)
        subprocess.run(cmake_config_cmd, check=True)

    print("Building sysroot components...")
    print(" ".join(cmake_build_cmd))
    if not args.dry_run:
        subprocess.run(cmake_build_cmd, check=True)

    print("Installing to sysroot...")
    print(" ".join(cmake_install_cmd))
    if not args.dry_run:
        subprocess.run(cmake_install_cmd, check=True)

    manifest_path = out_dir / "manifest.json"
    manifest = {
        "architecture": args.arch,
        "profile": args.profile,
        "build_type": args.build_type,
        "source_preset": preset_name
    }

    print(f"Generating manifest at {manifest_path}...")
    if not args.dry_run:
        os.makedirs(out_dir, exist_ok=True)
        with open(manifest_path, "w") as f:
            json.dump(manifest, f, indent=4)

    print(f"SDK sysroot correctly built at {out_dir}")

if __name__ == "__main__":
    build_sysroot()
