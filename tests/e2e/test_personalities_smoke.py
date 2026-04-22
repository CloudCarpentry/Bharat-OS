import sys
import os
import subprocess

def check_log_markers(log_file, markers):
    with open(log_file, 'r') as f:
        content = f.read()
        for marker in markers:
            if marker not in content:
                print(f"[FAIL] Missing marker '{marker}' in {log_file}")
                return False
    return True

def run_target(target, is_android):
    print(f"[*] Running E2E test for {target}")

    cmd = ["timeout", "15", "./build.sh", "all", "--target", target]

    log_dir = "e2e_logs_personalities"
    os.makedirs(log_dir, exist_ok=True)
    log_file = os.path.join(log_dir, f"{target}.log")

    with open(log_file, "w") as f:
        subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT)

    # Basic markers
    markers = ["BOOT: kernel_main reached", "Bharat-OS"]

    if check_log_markers(log_file, markers):
        print(f"[PASS] {target} booted successfully.")
        return True
    return False

def main():
    targets = [
        ("x86_64_desktop_headless_linux", False),
        ("x86_64_desktop_headless_android", True),
        ("arm64_desktop_headless_linux", False),
        ("arm64_desktop_headless_android", True),
        ("riscv64_desktop_headless_linux", False),
        ("riscv64_desktop_headless_android", True),
    ]

    all_passed = True
    for target, is_android in targets:
        if not run_target(target, is_android):
            all_passed = False

    if not all_passed:
        sys.exit(1)
    print("[SUCCESS] All personality E2E tests passed!")

if __name__ == "__main__":
    main()
