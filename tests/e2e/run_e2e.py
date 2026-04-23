#!/usr/bin/env python3
import sys
import os
import subprocess
import argparse
import time
import json

def parse_yaml_manual(filepath):
    """Fallback manual parser for a flat YAML file if PyYAML is not available."""
    data = {}
    with open(filepath, 'r') as f:
        lines = f.readlines()
        current_list_key = None
        for line in lines:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            if line.startswith('- '):
                if current_list_key:
                    item = line[2:].strip()
                    if item.startswith('"') and item.endswith('"'):
                        item = item[1:-1]
                    data[current_list_key].append(item)
                continue
            if ':' in line:
                key, val = line.split(':', 1)
                key = key.strip()
                val = val.strip()
                if not val:
                    current_list_key = key
                    data[key] = []
                else:
                    current_list_key = None
                    if val.startswith('"') and val.endswith('"'):
                        val = val[1:-1]
                    data[key] = val
    return data

def find_kernel_elf(build_dir):
    candidate = os.path.join(build_dir, "kernel", "kernel.elf")
    if os.path.isfile(candidate):
        return candidate
    for root, dirs, files in os.walk(build_dir):
        if "kernel.elf" in files:
            return os.path.join(root, "kernel.elf")
    return None

def get_objcopy_bin():
    try:
        subprocess.run(["llvm-objcopy", "--version"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        return "llvm-objcopy"
    except FileNotFoundError:
        pass
    try:
        subprocess.run(["objcopy", "--version"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        return "objcopy"
    except FileNotFoundError:
        return None

def main():
    parser = argparse.ArgumentParser(description="Run E2E test for a specific profile.")
    parser.add_argument("profile_file", help="Path to the profile YAML file")
    parser.add_argument("--timeout", type=int, default=15, help="QEMU timeout in seconds")
    parser.add_argument("--log-dir", default="e2e_logs", help="Directory to store logs")
    args = parser.parse_args()

    try:
        import yaml
        with open(args.profile_file, 'r') as f:
            profile = yaml.safe_load(f)
    except ImportError:
        print("[WARN] PyYAML not found, using manual fallback parser.")
        profile = parse_yaml_manual(args.profile_file)
    except Exception as e:
        print(f"[ERROR] Could not load profile {args.profile_file}: {e}")
        sys.exit(1)

    arch = profile.get('arch')
    device_profile = profile.get('device_profile')
    personality_profile = profile.get('personality_profile', 'NATIVE')
    memory_test = profile.get('memory_test', 'mmu')
    qemu_cmd = profile.get('qemu_cmd')
    qemu_args = profile.get('qemu_args', [])
    markers = profile.get('markers', [])

    if not all([arch, device_profile, qemu_cmd, markers]):
        print(f"[ERROR] Profile {args.profile_file} is missing required fields.")
        sys.exit(1)

    build_dir = f"build_e2e_{arch}_{memory_test}_{device_profile}_{personality_profile}"
    os.makedirs(args.log_dir, exist_ok=True)
    basename = os.path.basename(args.profile_file).replace('.yaml', '')
    log_file = os.path.join(args.log_dir, f"{basename}.log")

    print(f"[*] Starting E2E run for {basename} (arch={arch}, profile={device_profile})")

    # Determine memory test flags
    if memory_test in ["mmu", "iommu"]:
        mem_flags = ["-DBHARAT_PROFILE_MMU_FULL=ON", "-DBHARAT_PROFILE_MMU_LITE=OFF", "-DBHARAT_PROFILE_MPU_ONLY=OFF"]
    elif memory_test == "mmu-lite":
        mem_flags = ["-DBHARAT_PROFILE_MMU_FULL=OFF", "-DBHARAT_PROFILE_MMU_LITE=ON", "-DBHARAT_PROFILE_MPU_ONLY=OFF"]
    elif memory_test == "mpu":
        mem_flags = ["-DBHARAT_PROFILE_MMU_FULL=OFF", "-DBHARAT_PROFILE_MMU_LITE=OFF", "-DBHARAT_PROFILE_MPU_ONLY=ON"]
    else:
        print(f"[ERROR] Unsupported memory_test: {memory_test}")
        sys.exit(1)

    cmake_args = [
        "cmake", "-S", ".", "-B", build_dir,
        f"-DBHARAT_ARCH_FAMILY={arch}",
        f"-DBHARAT_DEVICE_PROFILE={device_profile}",
        f"-DBHARAT_PERSONALITY_PROFILE={personality_profile}"
    ] + mem_flags

    # Configure
    print(f"[*] Configuring CMake in {build_dir}...")
    result = subprocess.run(cmake_args, stdout=subprocess.DEVNULL)
    if result.returncode != 0:
        print(f"[FAIL] CMake configuration failed.")
        sys.exit(1)

    # Build
    print(f"[*] Building kernel.elf...")
    result = subprocess.run(["cmake", "--build", build_dir, "--target", "kernel.elf"], stdout=subprocess.DEVNULL)
    if result.returncode != 0:
        print(f"[FAIL] Build failed.")
        sys.exit(1)

    kernel_elf = find_kernel_elf(build_dir)
    if not kernel_elf:
        print(f"[FAIL] kernel.elf not found in {build_dir}.")
        sys.exit(1)

    kernel_image = kernel_elf

    print(f"[*] Running QEMU ({qemu_cmd}) with kernel {kernel_image}...")
    qemu_full_args = [qemu_cmd, "-kernel", kernel_image] + qemu_args

    with open(log_file, "w") as f:
        # Use timeout
        try:
            subprocess.run(qemu_full_args, stdout=f, stderr=subprocess.STDOUT, timeout=args.timeout)
        except subprocess.TimeoutExpired:
            # Expected to timeout if no-reboot doesn't auto-exit cleanly on success yet
            pass
        except Exception as e:
            print(f"[FAIL] QEMU execution error: {e}")
            sys.exit(1)

    print(f"[*] Asserting markers in log {log_file}...")
    assert_script = os.path.join(os.path.dirname(__file__), "assert_log.py")
    assert_args = [sys.executable, assert_script, log_file, "--markers"] + markers
    result = subprocess.run(assert_args)

    if result.returncode != 0:
        print(f"[FAIL] Assertions failed for {basename}.")
        sys.exit(1)

    print(f"[PASS] {basename} completed successfully.")

if __name__ == "__main__":
    main()
