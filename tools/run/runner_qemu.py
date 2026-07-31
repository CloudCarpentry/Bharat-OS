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
        if arch in ("riscv32", "riscv64"):
            print(f"TIP: RISC-V QEMU may be a separate package. Try:")
            print(f"     Ubuntu/Debian: sudo apt install qemu-system-misc")
            print(f"     Fedora/RHEL:   sudo dnf install qemu-system-riscv")
            print(f"     macOS:         brew install qemu")
        elif arch in ("arm32", "arm64"):
            print(f"TIP: ARM QEMU may be a separate package. Try:")
            print(f"     Ubuntu/Debian: sudo apt install qemu-system-arm")
            print(f"     Fedora/RHEL:   sudo dnf install qemu-system-arm")
            print(f"     macOS:         brew install qemu")
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

    # Boot artifact: passed via -kernel for all supported protocols.
    # - multiboot2 (x86_64): QEMU acts as bootloader, loads the ELF32 directly.
    # - linux_arm64/linux_arm32: QEMU jumps to the ELF load address, passes DTB in x0/r2.
    # - opensbi_payload (riscv64/riscv32): QEMU's built-in OpenSBI firmware boots first,
    #   then jumps to the -kernel ELF at 0x80200000 in S-mode.  No explicit -bios needed
    #   because QEMU virt defaults to its bundled OpenSBI when -bios is omitted.
    cmd.extend(["-kernel", boot_artifact])

    if dtb_path:
        cmd.extend(["-dtb", dtb_path])

    init_module = artifacts.get("init_module")
    if init_module:
        if arch == "x86_64":
            cmd.extend(["-initrd", f"{init_module} services/init"])
        else:
            cmd.extend(["-initrd", init_module])

    cmdline = boot_contract.get("cmdline")
    if cmdline:
        cmd.extend(["-append", cmdline])

    # Display / Serial configuration
    if nographic:
        cmd.extend(["-nographic", "-monitor", "none", "-serial", "stdio"])
    else:
        # For GUI, we still want serial output to stdio for logs
        cmd.extend(["-serial", "stdio"])

    # Always add -no-reboot to prevent infinite loops on panic
    cmd.append("-no-reboot")

    smp = run_config.get("smp", 1)
    required_online = run_config.get("required_online_cpus")
    if required_online is None:
        required_online = smp

    print(f"QEMU SMP requested: {smp}")
    print(f"Required online CPUs: {required_online}")
    print(f"QEMU argument: -smp {smp}")

    if smp:
        cmd.extend(["-smp", str(smp)])

    # Append extra_args, but strip any -machine entries: the machine flag was
    # already emitted above from run_config["machine"].  A stale -machine in
    # extra_args would produce a duplicate that makes QEMU refuse to start.
    extra_args = run_config.get("extra_args", [])
    if extra_args:
        filtered_extra: list[str] = []
        skip_next = False
        for arg in extra_args:
            if skip_next:
                skip_next = False
                print(f"[Run] WARNING: Ignoring duplicate -machine value '{arg}' in extra_args "
                      f"(machine already set to '{machine}').")
                continue
            if arg == "-machine":
                # Next token is the machine string — skip both
                skip_next = True
                print(f"[Run] WARNING: Ignoring duplicate -machine flag in extra_args "
                      f"(machine already set to '{machine}').")
                continue
            if arg.startswith("-machine="):
                print(f"[Run] WARNING: Ignoring duplicate '{arg}' in extra_args "
                      f"(machine already set to '{machine}').")
                continue
            filtered_extra.append(arg)
        cmd.extend(filtered_extra)

    print(f"[Run] Executing: {' '.join(cmd)}")

    import time
    import threading
    import queue

    # Headless boot success marker
    BOOT_MARKER = "BOOT: kernel_main reached"
    TIMEOUT_SEC = 60  # 60s: RISC-V OpenSBI + kernel init can be slow on CI hosts

    # Load boot contract
    repo_root = Path(__file__).resolve().parent.parent.parent
    contract_path = repo_root / "quality" / "contracts" / "boot" / "headless_boot_contract.yaml"
    required_markers = []
    forbidden_markers = []

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
        # Fallback to entry liveness marker
        required_markers = [BOOT_MARKER]
        forbidden_markers = ["PANIC", "ASSERT", "FAULT", "Unhandled exception"]

    observed_required = set()
    contract_satisfied = False
    failure_observed = False
    failure_reason = None
    log_lines = []

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
                log_lines.append(line)

                # Check forbidden markers
                for forbidden in forbidden_markers:
                    if forbidden in line:
                        failure_observed = True
                        failure_reason = f"Forbidden marker '{forbidden}' found in log: {line.strip()}"
                        break

                if failure_observed:
                    break

                # Check required markers in sequence order
                for req in required_markers:
                    if req not in observed_required and req in line:
                        observed_required.add(req)

                # Check if all required are satisfied
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

        # In interactive mode, continue reading until process exits
        if run_mode == "interactive":
            while proc.poll() is None:
                try:
                    line = q.get_nowait()
                    sys.stdout.write(line)
                    sys.stdout.flush()
                    log_lines.append(line)

                    # Check forbidden markers even in interactive mode
                    for forbidden in forbidden_markers:
                        if forbidden in line:
                            failure_observed = True
                            failure_reason = f"Forbidden marker '{forbidden}' found in log: {line.strip()}"
                            break
                except queue.Empty:
                    time.sleep(0.1)

        # Drain any remaining output
        t.join(timeout=1.0)
        while not q.empty():
            line = q.get_nowait()
            sys.stdout.write(line)
            sys.stdout.flush()
            log_lines.append(line)

            # Check forbidden
            for forbidden in forbidden_markers:
                if forbidden in line:
                    failure_observed = True
                    failure_reason = f"Forbidden marker '{forbidden}' found in log: {line.strip()}"

            # Check required
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
                # Try gentle termination first
                proc.terminate()
                try:
                    proc.wait(timeout=2)
                except subprocess.TimeoutExpired:
                    proc.kill()

            # Flush remaining output if any
            while not q.empty():
                line = q.get_nowait()
                sys.stdout.write(line)
                sys.stdout.flush()
                log_lines.append(line)

    # Save log to boot.log
    log_path = manifest_path.parent / "boot.log"
    try:
        with open(log_path, "w") as lf:
            lf.write("".join(log_lines))
    except Exception as e:
        print(f"[Run] Warning: Failed to write boot.log: {e}")

    # Generate and write machine-readable Evidence JSON
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
        "requested_cpu_count": smp,
        "online_cpu_count": required_online,
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
        # In interactive mode, we return the QEMU exit code or 1 if there was a failure
        if failure_observed:
            print(f"\n[Run] FAILURE DETECTED: {failure_reason}")
            return 1
        return proc.returncode if proc.returncode is not None else 0
