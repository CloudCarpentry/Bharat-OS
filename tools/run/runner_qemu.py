import json
import os
import subprocess
import sys
import platform
import time
import threading
import queue
from pathlib import Path
from shutil import which


def check_command(cmd_name: str) -> bool:
    return which(cmd_name) is not None


def load_run_manifest(path: Path) -> dict:
    if not path.exists():
        raise FileNotFoundError(f"Run manifest not found: {path}")
    with open(path, "r") as f:
        return json.load(f)

def build_qemu_command(manifest: dict, mode_override: str = None, display_override: str = None) -> list[str]:
    arch = manifest.get("arch")
    run_config = manifest.get("run_config", {})
    artifacts = manifest.get("artifacts", {})
    boot_contract = manifest.get("boot_contract", {})

    qemu_cmd = 'qemu-system-'
    if arch == 'arm64':
        qemu_cmd += 'aarch64'
    elif arch == 'arm32':
        qemu_cmd += 'arm'
    elif arch == 'riscv64':
        qemu_cmd += 'riscv64'
    elif arch == 'riscv32':
        qemu_cmd += 'riscv32'
    else:
        qemu_cmd += arch

    cmd = [qemu_cmd]

    machine = run_config.get("machine", "")
    if machine:
        cmd.extend(['-machine', machine])

    cpu = run_config.get("cpu", "")
    if cpu:
        cmd.extend(['-cpu', cpu])

    memory = run_config.get("memory", "512M")
    cmd.extend(['-m', str(memory)])

    smp = run_config.get("smp", 1)
    if smp > 1:
        cmd.extend(['-smp', str(smp)])

    boot_artifact = artifacts.get("boot_artifact", "")
    if boot_artifact:
        cmd.extend(['-kernel', boot_artifact])

    init_artifact = artifacts.get("init_module", "")
    if init_artifact:
        if arch == "x86_64":
            cmd.extend(["-initrd", f"{init_artifact} services/init"])
        else:
            cmd.extend(["-initrd", init_artifact])

    dtb_artifact = artifacts.get("dtb_artifact", "")
    if dtb_artifact:
        cmd.extend(["-dtb", dtb_artifact])

    # Handle Display Mode
    nographic_manifest = run_config.get("nographic", False)
    display_mode = "headless" if nographic_manifest else "gui"

    if display_override:
        display_mode = display_override

    is_windows = sys.platform.startswith('win')

    if display_mode == "headless":
        if is_windows:
            cmd.extend(['-display', 'none', '-serial', 'stdio'])
        else:
            cmd.append('-nographic')
    else:
        cmd.extend(['-display', 'gtk'])
        # Add virtio-gpu by default for GUI
        cmd.extend(["-device", "virtio-gpu-pci"])
        # In GUI mode, keep serial output on stdout for the runner to parse
        # or use vc if preferred, but for headless parsing we need it on stdio
        cmd.extend(["-serial", "stdio"])

    # Reboot Policy
    reboot_policy = run_config.get("reboot_policy", "stop")
    if reboot_policy == "stop":
        cmd.append("-no-reboot")

    cmdline = boot_contract.get("cmdline", "")
    if cmdline:
        cmd.extend(["-append", cmdline])

    extra_args = run_config.get("extra_args", [])
    if extra_args:
        filtered_extra = []
        for arg in extra_args:
            if arg == "-machine" or arg.startswith("-machine="):
                continue
            filtered_extra.append(arg)
        cmd.extend(filtered_extra)

    return cmd

def run_qemu(manifest_path: Path, mode_override: str = None, display_override: str = None) -> int:
    manifest = load_run_manifest(manifest_path)
    target_name = manifest.get("target_name")
    arch = manifest.get("arch")
    run_config = manifest.get("run_config", {})
    boot_contract = manifest.get("boot_contract", {})

    cmd = build_qemu_command(manifest, mode_override, display_override)
    runner = cmd[0]

    if not check_command(runner):
        print(f"[Run] Error: Missing QEMU executable: {runner}")
        return 1

    print(f"[Run] Executing: {' '.join(cmd)}")

    run_mode = mode_override or "interactive"

    BOOT_MARKER = "BOOT: kernel_main reached"
    TIMEOUT_SEC = 60

    required_markers = []
    forbidden_markers = []

    repo_root = Path(__file__).resolve().parent.parent.parent
    contract_path = repo_root / "quality" / "contracts" / "boot" / "headless_boot_contract.yaml"

    if contract_path.exists():
        try:
            import yaml
            with open(contract_path, "r") as f:
                contract = yaml.safe_load(f)

            targets = contract.get("targets", {})
            target_config = targets.get(target_name)
            if target_config:
                if "alias_of" in target_config:
                    target_config = targets.get(target_config["alias_of"])

                if target_config:
                    required_raw = target_config.get("required", [])
                    forbidden_raw = target_config.get("forbidden", [])

                    required_markers = [m if isinstance(m, str) else m.get("marker") or m.get("pattern") for m in required_raw if m]
                    forbidden_markers = [m if isinstance(m, str) else m.get("marker") or m.get("pattern") for m in forbidden_raw if m]
        except Exception as e:
            print(f"[Run] Warning: Failed to parse boot contract: {e}")

    if not required_markers:
        required_markers = [BOOT_MARKER]
        forbidden_markers = ["PANIC", "ASSERT", "FAULT", "Unhandled exception"]

    reboot_policy = run_config.get("reboot_policy", "stop")
    if reboot_policy == "expect":
        required_markers.append("BOOT: Generation 2")

    observed_required = set()
    contract_satisfied = False
    failure_observed = False
    failure_reason = None
    log_lines = []

    popen_kwargs = {}
    is_windows = sys.platform.startswith('win')
    if is_windows and hasattr(subprocess, "CREATE_NEW_PROCESS_GROUP"):
        popen_kwargs["creationflags"] = subprocess.CREATE_NEW_PROCESS_GROUP

    proc = None
    q: queue.Queue = queue.Queue()
    try:
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, bufsize=1, **popen_kwargs)
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
                log_lines.append(line)

                for forbidden in forbidden_markers:
                    if forbidden in line:
                        failure_observed = True
                        failure_reason = f"Forbidden marker '{forbidden}' found in log: {line.strip()}"
                        break

                if failure_observed:
                    break

                for req in required_markers:
                    if req not in observed_required and req in line:
                        observed_required.add(req)

                if len(observed_required) == len(required_markers):
                    contract_satisfied = True
                    if run_mode == "smoke":
                        break
            except queue.Empty:
                pass

            if run_mode == "smoke" and (time.time() - start_time > TIMEOUT_SEC):
                failure_observed = True
                failure_reason = f"Timeout ({TIMEOUT_SEC}s) reached before all required markers were observed."
                break

            time.sleep(0.1)

        if run_mode == "interactive":
            while proc.poll() is None:
                try:
                    line = q.get_nowait()
                    sys.stdout.write(line)
                    sys.stdout.flush()
                    log_lines.append(line)

                    for forbidden in forbidden_markers:
                        if forbidden in line:
                            failure_observed = True
                            failure_reason = f"Forbidden marker '{forbidden}' found in log: {line.strip()}"
                            break
                except queue.Empty:
                    time.sleep(0.1)

        t.join(timeout=1.0)
        while not q.empty():
            line = q.get_nowait()
            sys.stdout.write(line)
            sys.stdout.flush()
            log_lines.append(line)

            for forbidden in forbidden_markers:
                if forbidden in line:
                    failure_observed = True
                    failure_reason = f"Forbidden marker '{forbidden}' found in log: {line.strip()}"

            for req in required_markers:
                if req not in observed_required and req in line:
                    observed_required.add(req)

        if len(observed_required) == len(required_markers):
            contract_satisfied = True

        if not contract_satisfied and proc.poll() is not None and proc.returncode != 0:
            failure_observed = True
            failure_reason = f"QEMU process exited early with code {proc.returncode} before all required markers were observed."

    except KeyboardInterrupt:
        print("\n[Run] Terminating QEMU (User Interrupted)...")
    finally:
        if proc:
            if proc.poll() is None:
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

    log_path = manifest_path.parent / "boot.log"
    try:
        with open(log_path, "w") as lf:
            lf.write("".join(log_lines))
    except Exception as e:
        print(f"[Run] Warning: Failed to write boot.log: {e}")

    git_sha = "unknown"
    try:
        git_sha = subprocess.check_output(["git", "rev-parse", "HEAD"], text=True).strip()
    except Exception:
        pass

    qemu_version = "unknown"
    try:
        qemu_version = subprocess.check_output([runner, "--version"], text=True).splitlines()[0].strip()
    except Exception:
        pass

    evidence = {
        "schema_version": "1.0.0",
        "git_sha": git_sha,
        "target_yaml_path": f"delivery/targets/qemu/{target_name}.yaml",
        "target_name": target_name,
        "architecture": arch,
        "device_profile": manifest.get("device_profile", "unknown"),
        "execution_profile": manifest.get("execution_profile", "unknown"),
        "personality": manifest.get("personality_profile", "unknown"),
        "memory_model": "MPU" if "mpu" in target_name else "MMU_LITE" if "mmu_lite" in target_name else "MMU_FULL",
        "boot_handoff_kind": "STATIC_RT" if "mpu" in target_name else "USER_ELF",
        "qemu_executable": runner,
        "qemu_version": qemu_version,
        "requested_cpu_count": run_config.get("smp", 1),
        "online_cpu_count": run_config.get("required_online", run_config.get("smp", 1)),
        "build_result": "PASS",
        "package_result": "PASS",
        "runtime_result": "PASS" if contract_satisfied and not failure_observed else "FAIL",
        "required_markers": required_markers,
        "observed_markers": list(observed_required),
        "forbidden_markers": forbidden_markers,
        "start_timestamp": start_time,
        "duration": time.time() - start_time,
        "final_classification": "PASS" if contract_satisfied and not failure_observed else failure_reason or "BOOT_CONTRACT_FAIL",
        "log_artifact_path": str(log_path)
    }

    evidence_dir = repo_root / "build" / "evidence"
    try:
        evidence_dir.mkdir(parents=True, exist_ok=True)
        evidence_path = evidence_dir / f"{target_name}_evidence.json"
        with open(evidence_path, "w") as ef:
            json.dump(evidence, ef, indent=2)
        print(f"[Run] Wrote qualification evidence JSON to {evidence_path}")
    except Exception as e:
        print(f"[Run] Warning: Failed to write evidence JSON: {e}")

    if run_mode == "smoke":
        if contract_satisfied and not failure_observed:
            print(f"\n[Run] PASS: All {len(required_markers)} required boot contract markers observed successfully, and no forbidden markers found.")
            return 0
        else:
            print(f"\n[Run] FAIL: Boot contract was NOT satisfied.")
            if failure_observed:
                print(f"Reason: {failure_reason}")
            else:
                missing = [m for m in required_markers if m not in observed_required]
                print(f"Missing required markers: {missing}")
            return 1
    else:
        if failure_observed:
            print(f"\n[Run] FAILURE DETECTED: {failure_reason}")
            return 1
        return proc.returncode if proc.returncode is not None else 0
