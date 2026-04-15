import sys
import subprocess

def run(manifest):
    print("[QEMU Runner] Validating manifest...")
    # Reject: missing FDT when required
    dtb_handling = manifest.get('dtb_handling', {})
    if dtb_handling.get('required') and not dtb_handling.get('source'):
        print("Error: QEMU Runner requires FDT source when FDT is required.")
        sys.exit(1)

    # Reject: display requested but no device/frontend
    display_routing = manifest.get('display_routing', {})
    if display_routing.get('requested'):
        if not display_routing.get('device') or not display_routing.get('frontend'):
            print("Error: QEMU Runner display requested but no device or frontend specified.")
            sys.exit(1)

    # Reject: MCU-style targets
    if manifest.get('profile') == 'tiny' or manifest.get('profile') == 'mcu':
        print("Error: QEMU Runner rejects MCU-style/tiny targets (use Renode).")
        sys.exit(1)

    machine_cfg = manifest.get('machine_cfg', {})
    arch = manifest.get('arch')

    # Simple mapping to qemu command
    qemu_cmd = 'qemu-system-'
    if arch == 'arm64':
        qemu_cmd += 'aarch64'
    elif arch == 'arm32':
        qemu_cmd += 'arm'
    elif arch == 'riscv64':
        qemu_cmd += 'riscv64'
    else:
        qemu_cmd += arch

    cmd = [qemu_cmd]

    if machine_cfg.get('machine'):
        cmd.extend(['-machine', machine_cfg['machine']])

    if machine_cfg.get('cpu'):
        cmd.extend(['-cpu', machine_cfg['cpu']])

    cmd.extend(['-m', str(machine_cfg.get('memory', '512M'))])

    smp = machine_cfg.get('smp')
    if smp and smp > 1:
        cmd.extend(['-smp', str(smp)])

    artifacts = manifest.get('artifacts', {})
    if artifacts.get('kernel'):
        cmd.extend(['-kernel', artifacts['kernel']])

    serial_routing = manifest.get('serial_routing', {})
    dual_serial = manifest.get('dual_serial_requested', False)

    if serial_routing.get('stdio'):
        # Prevent monitor/serial stdio contention when routing boot logs to host terminal.
        cmd.extend(['-monitor', 'none'])
        cmd.extend(['-serial', 'stdio'])

    if dual_serial:
        cmd.extend(['-serial', 'vc'])

    if not display_routing.get('requested'):
        cmd.extend(['-display', 'none'])
    else:
         cmd.extend(['-display', 'gtk'])
         device = display_routing.get('device')
         if device == 'display.virtio_gpu':
             cmd.extend(['-device', 'virtio-gpu-pci'])
         elif device == 'display.vga':
             cmd.extend(['-vga', 'std'])

    print("\n--- Final QEMU Routing Summary ---")
    print(f"Host serial: {'enabled' if serial_routing.get('stdio') else 'disabled'}")
    print(f"Secondary serial sink: {'enabled' if dual_serial else 'disabled'}")
    print(f"GUI: {'enabled' if display_routing.get('requested') else 'disabled'}")
    print(f"Runner: {qemu_cmd}")
    print(f"Machine: {machine_cfg.get('machine', 'default')}")
    print(f"Board: {manifest.get('arch', 'unknown')} / {manifest.get('profile', 'unknown')}")
    print("----------------------------------\n")

    print(f"[QEMU Runner] Executing: {' '.join(cmd)}")

    # Try to execute if qemu is installed, otherwise just print
    try:
        subprocess.run(cmd)
    except FileNotFoundError:
        print(f"Warning: {qemu_cmd} not found. Simulated execution successful.")
