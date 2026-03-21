#!/usr/bin/env python3

import json
import os
import sys
import subprocess
import argparse

def normalize_csv_value(raw):
    return raw.split(',')[0].strip()

def doctor():
    tools = ['cmake', 'ninja', 'clang', 'lld', 'qemu-system-x86_64', 'qemu-system-aarch64', 'qemu-system-riscv64']
    print("Checking tools...")
    all_ok = True
    for tool in tools:
        path = subprocess.run(['which', tool], capture_output=True, text=True).stdout.strip()
        if path:
            print(f"  [OK] {tool}: {path}")
        else:
            print(f"  [MISSING] {tool}")
            all_ok = False

    if not all_ok:
        print("\nDoctor check failed: Some tools are missing.")
    else:
        print("\nDoctor check passed!")

def load_config(config_file="build_config.json"):
    try:
        with open(config_file, "r") as f:
            return json.load(f)
    except Exception as e:
        print(f"Error loading {config_file}: {e}")
        sys.exit(1)

def run_command(cmd, dry_run=False):
    if dry_run:
        print(f"DRY RUN: {' '.join(cmd)}")
        return 0
    else:
        print(f"Running: {' '.join(cmd)}")
        return subprocess.call(cmd)

def main():
    parser = argparse.ArgumentParser(description="Bharat-OS Build Wrapper")
    parser.add_argument("build_name", nargs="?", default="default_dev", help="Build name from build_config.json")
    parser.add_argument("--configure", action="store_true", help="Run CMake configure")
    parser.add_argument("--build", action="store_true", help="Run CMake build")
    parser.add_argument("--run", action="store_true", help="Run emulator")
    parser.add_argument("--test", action="store_true", help="Run CTest")
    parser.add_argument("--clean", action="store_true", help="Clean build directory")
    parser.add_argument("--list", action="store_true", help="List available builds")
    parser.add_argument("--doctor", action="store_true", help="Check dependencies")
    parser.add_argument("--dry-run", action="store_true", help="Print commands without running them")
    parser.add_argument("--dual-serial", action="store_true", help="Enable dual serial output")

    args, unknown = parser.parse_known_args()

    if args.doctor:
        doctor()
        return

    config_data = load_config()

    if args.list:
        print("Available builds:")
        for name in config_data.get("builds", {}):
            print(f"  - {name}")
        return

    build_name = args.build_name

    # Legacy flags from build.sh compatibility
    if build_name == "--run" or args.run:
        build_name = "default_dev"
        args.run = True
    elif build_name == "--dual-serial" or args.dual_serial:
        build_name = "default_dev"
        args.dual_serial = True

    # If no explicit command is given, default to configure + build
    if not (args.configure or args.build or args.run or args.test or args.clean):
        args.configure = True
        args.build = True

    builds = config_data.get("builds", {})
    if build_name not in builds:
        print(f"Build '{build_name}' not found in build_config.json")
        sys.exit(1)

    cfg = builds[build_name]
    preset = cfg.get("preset", "")
    arch = cfg.get("arch", "x86_64")
    profile = normalize_csv_value(cfg.get("profile", "DESKTOP")).upper()
    personality = normalize_csv_value(cfg.get("personality", "NONE")).upper()
    board = normalize_csv_value(cfg.get("board", ""))
    gui = cfg.get("gui", False)

    gui_flag = "ON" if gui else "OFF"

    build_dir = f"build/{preset}"

    if args.clean:
        run_command(["rm", "-rf", build_dir], dry_run=args.dry_run)
        return

    if args.configure:
        cmd = [
            "cmake", f"--preset={preset}",
            f"-DBHARAT_ARCH_FAMILY={arch}",
            f"-DBHARAT_DEVICE_PROFILE={profile}",
            f"-DBHARAT_PERSONALITY_PROFILE={personality}",
            f"-DBHARAT_TARGET_BOARD={board}",
            f"-DBHARAT_BOOT_GUI={gui_flag}"
        ]
        if run_command(cmd, dry_run=args.dry_run) != 0:
            sys.exit(1)

    if args.build:
        cmd = ["cmake", "--build", f"--preset={preset}"]
        if run_command(cmd, dry_run=args.dry_run) != 0:
            sys.exit(1)

    if args.test:
        cmd = ["ctest", "--preset", preset]
        if run_command(cmd, dry_run=args.dry_run) != 0:
            sys.exit(1)

    if args.run or cfg.get("run", False):
        qemu_opts = []
        if gui:
            if args.dual_serial:
                qemu_opts.extend(["-serial", "stdio", "-serial", "vc"])
            else:
                qemu_opts.extend(["-serial", "vc"])

            if arch == "x86_64":
                qemu_opts.extend(["-vga", "std"])
            elif arch == "arm64":
                qemu_opts.extend(["-device", "virtio-gpu-pci"])
            elif arch == "riscv64":
                qemu_opts.extend(["-device", "virtio-gpu-device", "-device", "ramfb"])
        else:
            qemu_opts.extend(["-serial", "stdio", "-display", "none"])

        kernel_path = f"build/{preset}/kernel.elf"

        if arch == "x86_64":
            cmd = ["qemu-system-x86_64", "-kernel", kernel_path, "-m", "512"] + qemu_opts
        elif arch == "arm64":
            cmd = ["qemu-system-aarch64", "-M", "virt", "-cpu", "cortex-a53", "-m", "512", "-kernel", kernel_path] + qemu_opts
        elif arch == "arm32" and board == "avh-corstone310":
            cmd = ["VHT_Corstone_SSE-310", "-a", kernel_path] + qemu_opts
        elif arch == "riscv64":
            cmd = ["qemu-system-riscv64", "-M", "virt", "-m", "512", "-kernel", kernel_path] + qemu_opts
        else:
            print(f"Unsupported architecture/board for running: {arch} / {board}")
            sys.exit(1)

        run_command(cmd, dry_run=args.dry_run)

if __name__ == "__main__":
    main()
