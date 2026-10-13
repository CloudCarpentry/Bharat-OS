import argparse
import subprocess
import sys
import os
import time
import json
import queue
import threading
from pathlib import Path

def map_target(target):
    if not target.endswith(".yaml"):
        if not target.startswith("delivery/targets/qemu/"):
            return f"delivery/targets/qemu/{target}.yaml"
    return target

def run(args):
    parser = argparse.ArgumentParser(description="Run a demo target")
    parser.add_argument("target", help="The target to run (e.g. x86_64_hmem_demo)")
    parsed_args, unknown = parser.parse_known_args(args)

    target_yaml = map_target(parsed_args.target)

    # 1. Build
    print(f"[Demo] Building {target_yaml}...")
    build_cmd = [sys.executable, "tools/nirmaan/cli.py", "build", target_yaml]
    result = subprocess.run(build_cmd)
    if result.returncode != 0:
        print("[Demo] Build failed.")
        return result.returncode

    # 2. Package
    print(f"[Demo] Packaging {target_yaml}...")
    # we can run tools/build.py package --target-yaml <target_yaml> directly
    pkg_cmd = [sys.executable, "tools/build.py", "package", "--target-yaml", target_yaml]
    result = subprocess.run(pkg_cmd)
    if result.returncode != 0:
        print("[Demo] Packaging failed.")
        return result.returncode

    # 3. Read run manifest
    # the target name is the base name without .yaml
    target_name = Path(target_yaml).stem

    repo_root = Path(__file__).resolve().parent.parent.parent.parent

    # We can infer build dir (CMake preset) from target YAML config directly to avoid brittle string matching
    # Alternatively parse the target yaml.
    target_yaml_path = repo_root / target_yaml

    preset_dir = target_name.replace("_", "-") # fallback
    if target_yaml_path.exists():
        import yaml
        with open(target_yaml_path, "r") as f:
            target_data = yaml.safe_load(f)
            # Find cmake_preset inside build
            if "build" in target_data and "cmake_preset" in target_data["build"]:
                preset_dir = target_data["build"]["cmake_preset"]

    run_manifest_path = repo_root / "build" / preset_dir / "manifests" / "run-manifest.json"


    if not run_manifest_path.exists():
        print(f"[Demo] Stale artifact or missing manifest at {run_manifest_path}")
        return 1

    with open(run_manifest_path, "r") as f:
        manifest = json.load(f)

    # 4. Run QEMU
    sys.path.insert(0, str(repo_root))
    from tools.run.runner_qemu import build_qemu_command
    qemu_cmd = build_qemu_command(manifest)
    if not qemu_cmd:
        print("[Demo] Failed to build QEMU command from manifest.")
        return 1

    print(f"[Demo] Executing QEMU: {' '.join(qemu_cmd)}")

    q = queue.Queue()
    proc = None
    log_lines = []

    is_windows = sys.platform.startswith('win')
    popen_kwargs = {}
    if is_windows and hasattr(subprocess, "CREATE_NEW_PROCESS_GROUP"):
        popen_kwargs["creationflags"] = subprocess.CREATE_NEW_PROCESS_GROUP

    start_time = time.time()
    timeout = 60

    last_verified_boot_milestone = "NONE"
    kernel_boot_status = "PENDING"
    build_status = "PASS"
    userspace_readiness = "NOT_VERIFIED"
    gui_visibility = "NOT_VERIFIED"
    interaction_status = "NOT_VERIFIED"
    failure_reason = None

    try:
        proc = subprocess.Popen(qemu_cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, bufsize=1, **popen_kwargs)

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

        failure_observed = False

        while proc.poll() is None:
            try:
                line = q.get_nowait()
                sys.stdout.write(line)
                sys.stdout.flush()
                log_lines.append(line)

                # Check for panic
                if "KERNEL PANIC" in line or "PANIC" in line:
                    failure_observed = True
                    failure_reason = f"Kernel panic detected: {line.strip()}"
                    kernel_boot_status = "BOOT_FAIL"
                    break

                if "BOOT: kernel_main reached" in line:
                    last_verified_boot_milestone = "KERNEL_MAIN"
                elif "BOOT_MEMORY: MODULES_RESERVED" in line:
                    last_verified_boot_milestone = "MEMORY_READY"
                elif "BOOT: KERNEL_RUNTIME_READY" in line:
                    last_verified_boot_milestone = "KERNEL_RUNTIME_READY"
                elif "BOOT: USERSPACE_LAUNCH_BEGIN" in line:
                    last_verified_boot_milestone = "USERSPACE_LAUNCH_BEGIN"
                elif "USERSPACE_READY" in line:
                    userspace_readiness = "PASS"

            except queue.Empty:
                pass

            if time.time() - start_time > timeout:
                failure_observed = True
                failure_reason = "Timeout exceeded before completing boot."
                kernel_boot_status = "TIMEOUT"
                break

            time.sleep(0.01)

    except Exception as e:
        failure_observed = True
        failure_reason = f"Exception during execution: {str(e)}"
        kernel_boot_status = "FAIL"
    finally:
        if proc and proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=2)
            except subprocess.TimeoutExpired:
                proc.kill()

        while not q.empty():
            line = q.get_nowait()
            sys.stdout.write(line)
            sys.stdout.flush()
            log_lines.append(line)

            if "KERNEL PANIC" in line or "PANIC" in line:
                if not failure_observed:
                    failure_observed = True
                    failure_reason = f"Kernel panic detected: {line.strip()}"
                    kernel_boot_status = "BOOT_FAIL"

    if proc and proc.poll() is not None:
        if proc.returncode != 0 and not failure_observed:
            failure_observed = True
            failure_reason = f"QEMU exited unexpectedly with code {proc.returncode}"
            kernel_boot_status = "FAIL"

    if kernel_boot_status not in ["BOOT_FAIL", "TIMEOUT", "FAIL"]:
        if "KERNEL PANIC" not in "".join(log_lines):
            kernel_boot_status = "PASS" if not failure_observed else "FAIL"

    # Never report success without actual evidence
    # (Requirement: Never report USERSPACE_READY from process launch alone... etc)
    # The requirement is that we must not assume success if it doesn't print it.

    evidence_dir = repo_root / "build" / "evidence"
    evidence_dir.mkdir(parents=True, exist_ok=True)
    evidence_path = evidence_dir / "demo_results.json"

    log_path = evidence_dir / "demo_boot.log"
    with open(log_path, "w") as f:
        f.write("".join(log_lines))

    result_json = {
        "build_status": build_status,
        "kernel_boot_status": kernel_boot_status,
        "last_verified_boot_milestone": last_verified_boot_milestone,
        "userspace_readiness": userspace_readiness,
        "gui_visibility": gui_visibility,
        "interaction_status": interaction_status,
        "failure_reason": failure_reason,
        "evidence_paths": [str(evidence_path), str(log_path)]
    }

    with open(evidence_path, "w") as f:
        json.dump(result_json, f, indent=2)

    print(f"[Demo] Wrote JSON result to {evidence_path}")

    if failure_observed:
        print(f"[Demo] Run FAILED: {failure_reason}")
        return 1

    print("[Demo] Run COMPLETED.")
    return 0
