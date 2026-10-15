"""
Renode Execution Runner for Bharat-OS
Unattended simulation pipeline that boots Bharat-OS on Renode emulated platforms,
connects to guest serial terminals over TCP socket, evaluates contract markers,
and produces structured test evidence.
"""

from __future__ import annotations

import json
import os
import platform
import queue
import shutil
import socket
import subprocess
import sys
import tempfile
import threading
import time
from pathlib import Path
from typing import Any, Dict, List, Optional, Set, Tuple


DEFAULT_TIMEOUT_SEC = 25


def find_renode_executable() -> Optional[Path]:
    """Discover the Renode executable on the host system."""
    env_path = os.environ.get("RENODE_PATH")
    if env_path and Path(env_path).is_file():
        return Path(env_path)

    # Check PATH
    for candidate in ("renode", "Renode.exe", "renode-test"):
        which_result = shutil.which(candidate)
        if which_result:
            p = Path(which_result)
            # If renode-test or bat on Windows, try resolving to Renode.exe
            if p.name.lower() in ("renode.exe", "renode"):
                return p
            renode_sibling = p.parent / "Renode.exe"
            if renode_sibling.is_file():
                return renode_sibling

    # Standard Windows install locations
    for win_candidate in (
        r"C:\Program Files\Renode\bin\Renode.exe",
        r"C:\Program Files (x86)\Renode\bin\Renode.exe",
        os.path.expandvars(r"%LOCALAPPDATA%\Programs\Renode\Renode.exe"),
    ):
        if Path(win_candidate).is_file():
            return Path(win_candidate)

    # Standard Linux / WSL locations
    for nix_candidate in (
        "/usr/bin/renode",
        "/usr/local/bin/renode",
        "/opt/renode/bin/renode",
        "/opt/renode/renode",
    ):
        if Path(nix_candidate).is_file():
            return Path(nix_candidate)

    return None


def find_free_port() -> int:
    """Find an available TCP port on localhost."""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind(("127.0.0.1", 0))
        s.listen(1)
        return s.getsockname()[1]


def resolve_platform_description(machine: str, repo_root: Path) -> str:
    """Resolve the platform description (.repl) string or path for Renode."""
    # Check if a custom platform exists in repo
    custom_repo_path = repo_root / "delivery" / "targets" / "renode" / f"{machine}.repl"
    if custom_repo_path.is_file():
        return str(custom_repo_path.resolve()).replace("\\", "/")

    tools_repl = repo_root / "tools" / "renode" / f"{machine}.repl"
    if tools_repl.is_file():
        return str(tools_repl.resolve()).replace("\\", "/")

    # Standard built-in Renode platform descriptions
    if machine in ("cortex-a53-gicv2", "cortex-a53-gicv3", "cortex-a78"):
        return f"@platforms/cpus/{machine}.repl"
    elif machine in ("stm32f4_discovery", "stm32f4_discovery-kit"):
        return "@platforms/boards/stm32f4_discovery.repl"
    elif machine.startswith("platforms/"):
        return f"@{machine}"

    # Default to boards or cpus lookup
    return f"@platforms/boards/{machine}.repl"


import struct


def build_minimal_dtb(
    initrd_start: int,
    initrd_size: int,
    ram_base: int = 0x40000000,
    ram_size: int = 0x40000000,
) -> bytes:
    """Generate a minimal valid FDT (Device Tree Blob) for ARM64 emulator booting."""
    strings = bytearray()

    def add_string(s: str) -> int:
        offset = len(strings)
        strings.extend(s.encode("ascii") + b"\x00")
        return offset

    off_compat = add_string("compatible")
    off_addr_cells = add_string("#address-cells")
    off_size_cells = add_string("#size-cells")
    off_device_type = add_string("device_type")
    off_reg = add_string("reg")
    off_bootargs = add_string("bootargs")
    off_initrd_start = add_string("linux,initrd-start")
    off_initrd_end = add_string("linux,initrd-end")

    FDT_BEGIN_NODE = 1
    FDT_END_NODE = 2
    FDT_PROP = 3
    FDT_END = 9

    dt_struct = bytearray()

    def begin_node(name: str):
        dt_struct.extend(struct.pack(">I", FDT_BEGIN_NODE))
        raw = name.encode("ascii") + b"\x00"
        dt_struct.extend(raw)
        pad = (4 - (len(raw) % 4)) % 4
        dt_struct.extend(b"\x00" * pad)

    def end_node():
        dt_struct.extend(struct.pack(">I", FDT_END_NODE))

    def add_prop(name_off: int, data: bytes):
        dt_struct.extend(struct.pack(">III", FDT_PROP, len(data), name_off))
        dt_struct.extend(data)
        pad = (4 - (len(data) % 4)) % 4
        dt_struct.extend(b"\x00" * pad)

    # Root node ""
    begin_node("")
    add_prop(off_compat, b"linux,dummy-virt\x00")
    add_prop(off_addr_cells, struct.pack(">I", 2))
    add_prop(off_size_cells, struct.pack(">I", 2))

    # memory node
    begin_node("memory@40000000")
    add_prop(off_device_type, b"memory\x00")
    add_prop(off_reg, struct.pack(">QQ", ram_base, ram_size))
    end_node()

    # chosen node
    begin_node("chosen")
    add_prop(off_bootargs, b"console=uart0\x00")
    if initrd_start and initrd_size:
        add_prop(off_initrd_start, struct.pack(">Q", initrd_start))
        add_prop(off_initrd_end, struct.pack(">Q", initrd_start + initrd_size))
    end_node()

    end_node()  # end root node
    dt_struct.extend(struct.pack(">I", FDT_END))

    mem_rsvmap = struct.pack(">QQ", 0, 0)

    header_size = 40
    off_mem_rsvmap = header_size
    off_dt_struct = off_mem_rsvmap + len(mem_rsvmap)
    off_dt_strings = off_dt_struct + len(dt_struct)
    total_size = off_dt_strings + len(strings)

    header = struct.pack(
        ">IIIIIIIIII",
        0xD00DFEED,      # magic
        total_size,      # totalsize
        off_dt_struct,   # off_dt_struct
        off_dt_strings,  # off_dt_strings
        off_mem_rsvmap,  # off_mem_rsvmap
        17,              # version
        16,              # last_comp_version
        0,               # boot_cpuid_phys
        len(strings),    # size_dt_strings
        len(dt_struct),  # size_dt_struct
    )

    return header + mem_rsvmap + dt_struct + strings


def build_resc_script(
    manifest: Dict[str, Any],
    port: int,
    repo_root: Path,
    script_path: Path,
) -> str:
    """Generate a Renode .resc execution script from the run manifest."""
    run_cfg = manifest.get("run_config", {})
    artifacts = manifest.get("artifacts", {})
    boot_contract = manifest.get("boot_contract", {})

    machine = run_cfg.get("machine", "cortex-a53-gicv2")
    platform_desc = resolve_platform_description(machine, repo_root)

    boot_artifact = artifacts.get("boot_artifact") or artifacts.get("canonical_elf") or artifacts.get("kernel")
    if not boot_artifact:
        raise ValueError("Renode Runner requires a boot_artifact or canonical_elf in manifest.")

    boot_artifact_path = Path(boot_artifact).resolve()
    boot_artifact_str = str(boot_artifact_path).replace("\\", "/")

    init_artifact = artifacts.get("init_module")
    init_path_str = ""
    init_size = 0
    if init_artifact:
        ip = Path(init_artifact).resolve()
        if ip.is_file():
            init_path_str = str(ip).replace("\\", "/")
            init_size = ip.stat().st_size

    dtb_path_str = ""
    dtb_artifact = artifacts.get("dtb_artifact") or artifacts.get("dtb_path")
    if dtb_artifact:
        p = Path(dtb_artifact).resolve()
        if p.is_file():
            dtb_path_str = str(p).replace("\\", "/")


    # Determine primary serial UART peripheral in Renode
    serial_list = run_cfg.get("serial", [])
    primary_uart = serial_list[0] if serial_list else ("usart2" if "stm32" in machine else "uart0")
    if not primary_uart.startswith("sysbus."):
        uart_peripheral = f"sysbus.{primary_uart}"
    else:
        uart_peripheral = primary_uart

    lines = [
        ":name: Bharat-OS Renode Automated Run",
        "using sysbus",
        "mach create",
        f"machine LoadPlatformDescription {platform_desc}",
    ]

    # Load init_module binary into RAM at 0x48000000 if present
    if init_path_str and "stm32" not in machine:
        lines.append(f"sysbus LoadBinary @{init_path_str} 0x48000000")

    # If DTB is required or auto-generated, load binary at standard RAM offset 0x40000000
    if dtb_path_str and "stm32" not in machine:
        lines.append(f"sysbus LoadBinary @{dtb_path_str} 0x40000000")

    # Load ELF
    lines.append(f"sysbus LoadELF @{boot_artifact_str}")

    # Pass DTB register if needed (x0 / r2 = 0x40000000)
    if dtb_path_str and "stm32" not in machine:
        lines.append("cpu SetRegister 0 0x40000000")

    # Create headless TCP server socket terminal and connect to target UART
    lines.extend([
        f'emulation CreateServerSocketTerminal {port} "guest_uart_term"',
        f'connector Connect {uart_peripheral} guest_uart_term',
        "start",
    ])

    content = "\n".join(lines) + "\n"
    script_path.write_text(content, encoding="utf-8")
    return content


def run_renode(
    manifest_input: Any,
    mode_override: Optional[str] = None,
    display_override: Optional[str] = None,
    timeout_sec: int = DEFAULT_TIMEOUT_SEC,
) -> int:
    """
    Execute a target run manifest under Antmicro Renode.
    Streams UART logs, verifies contract markers, enforces bounded timeouts,
    and writes boot.log and evidence JSON.
    """
    # Load manifest
    manifest_path: Optional[Path] = None
    if isinstance(manifest_input, (str, Path)):
        manifest_path = Path(manifest_input).resolve()
        if not manifest_path.is_file():
            print(f"[Renode Runner] Error: Manifest file not found: {manifest_path}")
            return 1
        with open(manifest_path, "r", encoding="utf-8") as f:
            manifest = json.load(f)
    elif isinstance(manifest_input, dict):
        manifest = manifest_input
    else:
        print("[Renode Runner] Error: Invalid manifest input type.")
        return 1

    print("[Renode Runner] Validating manifest...")

    # Validate runner type
    runner_type = manifest.get("runner_type")
    if runner_type and runner_type != "renode":
        print(f"[Renode Runner] Error: Expected runner_type 'renode', got '{runner_type}'.")
        return 1

    # Reject desktop PCI assumptions on embedded/renode targets
    devices = manifest.get("device_list", [])
    if any("pci" in str(d).lower() for d in devices):
        print("[Renode Runner] Error: Renode Runner rejects desktop PCI assumptions.")
        return 1

    artifacts = manifest.get("artifacts", {})
    boot_artifact = artifacts.get("boot_artifact") or artifacts.get("canonical_elf") or artifacts.get("kernel")
    if not boot_artifact:
        print("[Renode Runner] Error: Renode Runner requires boot_artifact or canonical_elf.")
        return 1

    boot_path = Path(boot_artifact)
    if not boot_path.is_file():
        # Check relative to manifest directory if provided
        if manifest_path and (manifest_path.parent / boot_artifact).is_file():
            boot_path = manifest_path.parent / boot_artifact
        else:
            print(f"[Renode Runner] Error: Boot artifact file does not exist: {boot_artifact}")
            return 1

    # Discover Renode binary
    renode_bin = find_renode_executable()
    if not renode_bin:
        print("[Renode Runner] Error: Renode executable not found.")
        print("  Install Renode or set the RENODE_PATH environment variable.")
        return 1

    print(f"[Renode Runner] Discovered Renode: {renode_bin}")

    # Determine repo root
    repo_root = Path(__file__).resolve().parent.parent.parent

    # Contract markers
    target_name = manifest.get("target_name", "renode_guest")
    required_markers: List[str] = [
        "BOOT: kernel_main reached",
    ]
    forbidden_markers: List[str] = [
        "KERNEL PANIC",
        "EXCEPTION",
        "BOOT_FAIL:",
        "ASSERTION FAILED",
        "gpioPortC: Attempted Byte write isn't supported",
    ]

    # Add ECU markers if testing ECU workload
    if "ecu" in target_name.lower():
        required_markers.extend([
            "[ECU_FAN] INIT: Cooling fan controller online",
            "[ECU_FAN] STATE=HOT",
            "[ECU_FAN] STATE=COOLING",
            "[ECU_FAN] SENSOR_FAILURE",
            "[ECU_FAN] STATE=SAFE_STATE",
            "[ECU_FAN] SCENARIO_COMPLETE: PASS",
        ])

    # Allocate ephemeral port
    port = find_free_port()

    # Determine run directory
    if manifest_path:
        run_dir = manifest_path.parent
    else:
        run_dir = repo_root / "build" / target_name

    run_dir.mkdir(parents=True, exist_ok=True)
    resc_file = run_dir / "renode_execution.resc"

    try:
        resc_content = build_resc_script(manifest, port, repo_root, resc_file)
    except Exception as exc:
        print(f"[Renode Runner] Error generating .resc script: {exc}")
        return 1

    print(f"[Renode Runner] Generated script at {resc_file}")

    # Launch Renode in unattended headless mode
    renode_cmd = [
        str(renode_bin),
        "--plain",
        "--disable-gui",
        "--console",
        str(resc_file),
    ]

    log_lines: List[str] = []
    observed_required: Set[str] = set()
    failure_observed = False
    failure_reason: Optional[str] = None
    start_time = time.time()

    proc: Optional[subprocess.Popen] = None
    client_sock: Optional[socket.socket] = None
    q: queue.Queue = queue.Queue()

    try:
        proc = subprocess.Popen(
            renode_cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
        )

        def renode_stdout_reader(pipe):
            try:
                for line in pipe:
                    q.put(("STDOUT", line))
            except Exception:
                pass
            finally:
                pipe.close()

        def socket_reader(sock):
            buf = ""
            try:
                while True:
                    data = sock.recv(1024)
                    if not data:
                        break
                    text = data.decode("latin1", errors="replace")
                    buf += text
                    while "\n" in buf:
                        line, buf = buf.split("\n", 1)
                        q.put(("UART", line + "\n"))
                if buf:
                    q.put(("UART", buf + "\n"))
            except Exception:
                pass

        t_out = threading.Thread(target=renode_stdout_reader, args=(proc.stdout,), daemon=True)
        t_out.start()

        # Connect client socket with retry loop
        connected = False
        connect_deadline = time.time() + 10.0
        while time.time() < connect_deadline and proc.poll() is None:
            try:
                client_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                client_sock.connect(("127.0.0.1", port))
                client_sock.settimeout(1.0)
                connected = True
                break
            except (ConnectionRefusedError, OSError):
                time.sleep(0.2)

        if connected and client_sock:
            t_sock = threading.Thread(target=socket_reader, args=(client_sock,), daemon=True)
            t_sock.start()
        elif proc.poll() is not None:
            failure_observed = True
            failure_reason = f"Renode process exited early with code {proc.returncode} before terminal connected."

        # Main evaluation loop
        while proc.poll() is None or not q.empty():
            try:
                source, line = q.get_nowait()
                sys.stdout.write(f"[{source}] {line}")
                sys.stdout.flush()
                log_lines.append(line)

                # Check forbidden markers
                for f_m in forbidden_markers:
                    if f_m in line:
                        failure_observed = True
                        failure_reason = f"Forbidden marker '{f_m}' observed in log: {line.strip()}"
                        break

                if failure_observed:
                    break

                # Check required markers
                for r_m in required_markers:
                    if r_m not in observed_required and r_m in line:
                        observed_required.add(r_m)
                        print(f"[Renode Runner] Observed required marker: {r_m}")

                if len(observed_required) == len(required_markers):
                    print("[Renode Runner] All required markers observed! Emulation contract satisfied.")
                    break

            except queue.Empty:
                pass

            if time.time() - start_time > timeout_sec:
                failure_observed = True
                missing = [m for m in required_markers if m not in observed_required]
                failure_reason = f"Timeout ({timeout_sec}s) exceeded. Missing markers: {missing}"
                break

            time.sleep(0.05)

    except KeyboardInterrupt:
        print("\n[Renode Runner] Terminated by user.")
        failure_observed = True
        failure_reason = "Execution interrupted by user."
    finally:
        if client_sock:
            try:
                client_sock.close()
            except Exception:
                pass

        if proc:
            if proc.poll() is None:
                proc.terminate()
                try:
                    proc.wait(timeout=2)
                except subprocess.TimeoutExpired:
                    proc.kill()

    duration = time.time() - start_time

    # Drain remaining queue
    while not q.empty():
        try:
            source, line = q.get_nowait()
            log_lines.append(line)
            for f_m in forbidden_markers:
                if f_m in line:
                    failure_observed = True
                    if not failure_reason:
                        failure_reason = f"Forbidden marker '{f_m}' observed in log."
            for r_m in required_markers:
                if r_m not in observed_required and r_m in line:
                    observed_required.add(r_m)
        except queue.Empty:
            break

    # Determine final outcome
    if not failure_observed and len(observed_required) == len(required_markers):
        runtime_result = "PASS"
        exit_code = 0
    else:
        runtime_result = "FAIL"
        exit_code = 1
        if not failure_reason:
            missing = [m for m in required_markers if m not in observed_required]
            failure_reason = f"Failed to observe all required markers: {missing}"

    # Write boot.log
    log_path = run_dir / "boot.log"
    try:
        log_path.write_text("".join(log_lines), encoding="utf-8")
        print(f"[Renode Runner] Wrote guest log to {log_path}")
    except Exception as exc:
        print(f"[Renode Runner] Warning: Failed to write boot.log: {exc}")

    # Write evidence JSON
    evidence_dir = repo_root / "build" / "evidence"
    evidence_dir.mkdir(parents=True, exist_ok=True)
    evidence_file = evidence_dir / f"{target_name}_evidence.json"

    evidence_data = {
        "target_name": target_name,
        "runner": "renode",
        "renode_binary": str(renode_bin),
        "runtime_result": runtime_result,
        "failure_reason": failure_reason,
        "required_markers": required_markers,
        "observed_required_markers": list(observed_required),
        "forbidden_markers": forbidden_markers,
        "duration_seconds": round(duration, 3),
        "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
    }

    try:
        evidence_file.write_text(json.dumps(evidence_data, indent=2), encoding="utf-8")
        print(f"[Renode Runner] Emitted verification evidence to {evidence_file}")
    except Exception as exc:
        print(f"[Renode Runner] Warning: Failed to write evidence JSON: {exc}")

    if runtime_result == "PASS":
        print(f"[Renode Runner] Execution successful in {duration:.2f}s.")
    else:
        print(f"[Renode Runner] Execution FAILED: {failure_reason}")

    return exit_code


def run(manifest: Any) -> None:
    """Legacy entry point signature for existing runner interface."""
    code = run_renode(manifest)
    if code != 0:
        sys.exit(code)


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python runner_renode.py <run-manifest.json>")
        sys.exit(1)
    sys.exit(run_renode(sys.argv[1]))
