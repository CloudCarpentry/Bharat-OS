import os
import sys
import json
import subprocess

def check_command(cmd_name):
    from shutil import which
    return which(cmd_name) is not None

def run(manifest):
    """
    Runs QEMU purely based on the declarative run-manifest.json.
    """
    print(f"\n[Run] Launching QEMU for {manifest.get('target_name')}...")

    arch = manifest.get('arch')
    run_config = manifest.get('run_config', {})
    artifacts = manifest.get('artifacts', {})
    boot_contract = manifest.get('boot_contract', {})

    # Validation: Boot contract compatibility
    protocol = boot_contract.get('protocol')
    boot_artifact = artifacts.get('boot_artifact')

    if not os.path.exists(boot_artifact):
        print(f"Error: Boot artifact not found: {boot_artifact}")
        sys.exit(1)

    print(f"[Run] Using boot artifact: {boot_artifact}")

    # Determine the correct runner binary
    qemu_bin_map = {
        'x86_64': 'qemu-system-x86_64',
        'arm64': 'qemu-system-aarch64',
        'riscv64': 'qemu-system-riscv64',
        'arm32': 'qemu-system-arm',
        'riscv32': 'qemu-system-riscv32'
    }

    runner = qemu_bin_map.get(arch)
    if not runner:
        print(f"Error: Unknown architecture '{arch}' for QEMU runner.")
        sys.exit(1)

    if not check_command(runner):
        print(f"\nERROR: Runner '{runner}' not found in PATH.")
        sys.exit(1)

    cmd = [runner]

    # Basic Machine/CPU/Memory flags
    machine = run_config.get('machine')
    if machine:
        cmd.extend(['-machine', machine])

    cpu = run_config.get('cpu')
    if cpu:
        cmd.extend(['-cpu', cpu])

    memory = run_config.get('memory')
    if memory:
        cmd.extend(['-m', memory])

    # Boot specific logic depending on contract / emulator
    if arch == 'arm64' and boot_contract.get('dtb', {}).get('mode') == 'qemu_generated':
        cmd.extend(['-bios', 'none'])

    if protocol == 'linux_arm64' or protocol == 'multiboot2' or protocol == 'opensbi_payload':
        cmd.extend(['-kernel', boot_artifact])
    else:
        # Default fallback if protocol is generic or not specified
        cmd.extend(['-kernel', boot_artifact])

    # Display / Serial
    display = run_config.get('display')
    if display == 'none':
        cmd.extend(['-nographic'])
    elif display == 'desktop' or display == 'mobile':
        cmd.extend(['-device', 'virtio-gpu-pci'])

    # SMP
    smp = run_config.get('smp')
    if smp:
        cmd.extend(['-smp', str(smp)])

    # Extra arguments
    extra_args = run_config.get('extra_args', [])
    cmd.extend(extra_args)

    print(f"[Run] Executing: {' '.join(cmd)}")

    try:
        proc = subprocess.Popen(cmd)
        proc.wait()
    except KeyboardInterrupt:
        print("\n[Run] Terminating QEMU...")
        proc.kill()
        proc.wait()
        sys.exit(0)

    if proc.returncode != 0:
        print(f"[Run] QEMU exited with code {proc.returncode}")
        sys.exit(proc.returncode)
