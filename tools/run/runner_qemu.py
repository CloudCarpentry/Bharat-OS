import json
import os
import subprocess
import sys
import platform
from pathlib import Path
from shutil import which


def check_command(cmd_name: str) -> bool:
    return which(cmd_name) is not None


def load_run_manifest(path: Path) -> dict:
    if not path.exists():
        raise FileNotFoundError(f"Run manifest not found: {path}")
    with open(path, "r") as f:
        return json.load(f)


def run_qemu(manifest_path: Path, mode_override: str = None, display_override: str = None) -> int:
    """
    Launches QEMU based on a run manifest and optional CLI overrides.

    :param manifest_path: Path to the run-manifest.json
    :param mode_override: 'smoke' or 'interactive'
    :param display_override: 'gui' or 'headless'
    """
    manifest = load_run_manifest(manifest_path)

    target_name = manifest.get("target_name")

    arch = manifest.get("arch")
    run_config = manifest.get("run_config", {})
    artifacts = manifest.get("artifacts", {})
    boot_contract = manifest.get("boot_contract", {})

    # Determine Display Mode
    # Priority: CLI override > Manifest config
    nographic_manifest = run_config.get("nographic", False)
    if display_override == "gui":
        nographic = False
    elif display_override == "headless":
        nographic = True
    else:
        nographic = nographic_manifest

    # Determine Run Mode
    # Priority: CLI override > Default based on display
    if mode_override:
        run_mode = mode_override
    else:
        # Default: GUI targets are interactive, headless targets are smoke
        run_mode = "interactive" if not nographic else "smoke"

    print(f"\n[Run] Launching QEMU for {target_name} ({run_mode} mode, {'headless' if nographic else 'gui'})...")

    boot_artifact = artifacts.get("boot_artifact")

    if not boot_artifact or not os.path.exists(boot_artifact):
        print(f"Error: Boot artifact not found: {boot_artifact}")
        sys.exit(1)

    print(f"[Run] Using boot artifact: {boot_artifact}")

    qemu_bin_map = {
        "x86_64": "qemu-system-x86_64",
        "arm64": "qemu-system-aarch64",
        "riscv64": "qemu-system-riscv64",
        "arm32": "qemu-system-arm",
        "riscv32": "qemu-system-riscv32",
    }

    runner_base = qemu_bin_map.get(arch)
    if not runner_base:
        print(f"Error: Unknown architecture '{arch}' for QEMU runner.")
        sys.exit(1)

    # Windows support for .exe suffix
    runners_to_try = [runner_base]
    if platform.system() == "Windows":
        runners_to_try.insert(0, f"{runner_base}.exe")

    runner = None
    for r in runners_to_try:
        if check_command(r):
            runner = r
            break

    if not runner:
        tried = " or ".join(runners_to_try)
        print(f"\nERROR: QEMU runner '{tried}' not found in PATH.")
        if platform.system() == "Windows":
             print("TIP: Ensure QEMU is installed and added to your System PATH.")
        sys.exit(1)

    cmd = [runner]

    machine = run_config.get("machine")
    if machine:
        cmd.extend(["-machine", machine])

    cpu = run_config.get("cpu")
    if cpu:
        cmd.extend(["-cpu", cpu])

    memory = run_config.get("memory")
    if memory:
        cmd.extend(["-m", memory])

    protocol = boot_contract.get("protocol")
    dtb_info = boot_contract.get("dtb", {})
    dtb_path = dtb_info.get("path")

    if dtb_info.get("required") and not dtb_path:
        print("Error: QEMU Runner requires DTB path but none was provided.")
        sys.exit(1)

    # Basic generic boot logic based on protocol
    cmd.extend(["-kernel", boot_artifact])

    if dtb_path:
        cmd.extend(["-dtb", dtb_path])

    # Display / Serial configuration
    if nographic:
        cmd.extend(["-nographic", "-monitor", "none", "-serial", "stdio"])
    else:
        # For GUI, we still want serial output to stdio for logs
        cmd.extend(["-serial", "stdio"])

    # Always add -no-reboot to prevent infinite loops on panic
    cmd.append("-no-reboot")

    smp = run_config.get("smp", 1)
    if smp:
        cmd.extend(["-smp", str(smp)])

    extra_args = run_config.get("extra_args", [])
    if extra_args:
        cmd.extend(extra_args)

    print(f"[Run] Executing: {' '.join(cmd)}")

    import time
    import threading
    import queue

    # Headless boot success marker
    BOOT_MARKER = "BOOT: kernel_main reached"
    TIMEOUT_SEC = 30
    marker_observed = False

    def _report_boot_success(name: str, marker: str) -> None:
        print(f"\n[Run] Boot marker observed: {marker}")
        print(f"[Run] PASS: {name} booted successfully")

    proc = None
    q: queue.Queue = queue.Queue()
    try:
        # Use a pipe for stdout to monitor serial output
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, bufsize=1)

        start_time = time.time()

        def reader(pipe, out_q):
            try:
                for line in pipe:
                    out_q.put(line)
            except OSError:
                pass
            finally:
                pipe.close()

        t = threading.Thread(target=reader, args=(proc.stdout, q))
        t.daemon = True
        t.start()

        while proc.poll() is None:
            try:
                line = q.get_nowait()
                sys.stdout.write(line)
                sys.stdout.flush()
                if BOOT_MARKER in line:
                    _report_boot_success(target_name, BOOT_MARKER)
                    marker_observed = True
                    if run_mode == "smoke":
                        break
            except queue.Empty:
                pass

            if run_mode == "smoke" and (time.time() - start_time > TIMEOUT_SEC):
                print(f"\n[Run] FAIL: Timeout ({TIMEOUT_SEC}s) reached before boot marker.")
                break

            time.sleep(0.1)

        # In interactive mode, continue reading until process exits
        if run_mode == "interactive":
            while proc.poll() is None:
                try:
                    line = q.get_nowait()
                    sys.stdout.write(line)
                    sys.stdout.flush()
                except queue.Empty:
                    time.sleep(0.1)

        # Drain any remaining output
        t.join(timeout=1.0)
        while not q.empty():
            line = q.get_nowait()
            sys.stdout.write(line)
            sys.stdout.flush()
            if BOOT_MARKER in line and not marker_observed:
                _report_boot_success(target_name, BOOT_MARKER)
                marker_observed = True

    except KeyboardInterrupt:
        print("\n[Run] Terminating QEMU (User Interrupted)...")
    finally:
        if proc:
            if proc.poll() is None:
                # Try gentle termination first
                proc.terminate()
                try:
                    proc.wait(timeout=2)
                except subprocess.TimeoutExpired:
                    proc.kill()

            # Flush remaining output if any
            while not q.empty():
                sys.stdout.write(q.get_nowait())
                sys.stdout.flush()

    if run_mode == "smoke":
        return 0 if marker_observed else 1
    else:
        # In interactive mode, we return the QEMU exit code
        return proc.returncode if proc.returncode is not None else 0
