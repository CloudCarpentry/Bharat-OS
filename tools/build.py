import argparse
import json
import os
import subprocess
import sys
import boot

from targets.loader import get_target
from targets.validate import validate_target

def load_manifest():
    manifest_path = os.path.join(os.path.dirname(os.path.dirname(__file__)), 'build_config.json')
    try:
        with open(manifest_path, 'r') as f:
            return json.load(f)
    except Exception as e:
        print(f"Error loading {manifest_path}: {e}")
        sys.exit(1)

def print_list(manifest):
    print("Available builds:")
    if 'builds' in manifest:
        for name, cfg in manifest['builds'].items():
            print(f"  {name}: {cfg.get('preset')} ({cfg.get('arch')} / {cfg.get('profile')})")
    else:
        print("  No builds configured.")

def check_command(cmd):
    try:
        subprocess.run([cmd, '--version'], capture_output=True, check=True)
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False

def check_qemu(arch):
    if arch == 'x86_64':
        return check_command('qemu-system-x86_64')
    elif arch == 'arm64':
        return check_command('qemu-system-aarch64')
    elif arch == 'riscv64':
        return check_command('qemu-system-riscv64')
    elif arch == 'riscv32':
        return check_command('qemu-system-riscv32')
    elif arch == 'arm32':
        # Prefer AVH runner if available, fallback to QEMU arm
        if check_command('VHT_Corstone_SSE-310'):
            return True
        return check_command('qemu-system-arm')
    return False

def doctor():
    print("Checking dependencies...")
    deps = {
        'cmake': check_command('cmake'),
        'ninja': check_command('ninja'),
        'clang': check_command('clang'),
        'lld': check_command('lld')
    }
    for tool, found in deps.items():
        print(f"  {tool}: {'Found' if found else 'Missing'}")

    print("\nChecking runners...")
    runners = ['x86_64', 'arm64', 'riscv64', 'arm32', 'riscv32']
    for arch in runners:
        found = check_qemu(arch)
        print(f"  QEMU/Runner ({arch}): {'Found' if found else 'Missing'}")

def normalize_csv(val):
    if not val:
        return ""
    return str(val).split(',')[0].strip().upper()

def run_command(cmd, cwd=None):
    print(f"Running: {' '.join(cmd)}")
    result = subprocess.run(cmd, cwd=cwd)
    if result.returncode != 0:
        print(f"Command failed with exit code {result.returncode}")
        sys.exit(result.returncode)

def check_and_clean_stale_cache(preset):
    build_dir = os.path.join('build', preset)
    cache_file = os.path.join(build_dir, 'CMakeCache.txt')
    if os.path.exists(cache_file):
        try:
            with open(cache_file, 'r') as f:
                for line in f:
                    if line.startswith('CMAKE_HOME_DIRECTORY:INTERNAL='):
                        cached_src_dir = line.split('=', 1)[1].strip()
                        current_src_dir = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))

                        # Replace backslashes with forward slashes to normalize C:\ vs C:/
                        norm_cached = cached_src_dir.replace('\\', '/')
                        norm_current = current_src_dir.replace('\\', '/')

                        # Use lowercase to handle case-insensitive paths (e.g. c:/ vs C:/)
                        if os.path.normcase(norm_cached) != os.path.normcase(norm_current):
                            print(f"Detected mixed-host build environment (cache: {cached_src_dir}, current: {current_src_dir}).")
                            print(f"Cleaning stale cache to avoid path contamination errors...")
                            import shutil
                            shutil.rmtree(build_dir, ignore_errors=True)
                        break
        except Exception as e:
            print(f"Warning: Failed to parse CMakeCache.txt: {e}")

def do_configure(build_cfg):
    preset = build_cfg.get('preset')
    arch = build_cfg.get('arch')
    profile = normalize_csv(build_cfg.get('profile', 'DESKTOP'))
    personality = normalize_csv(build_cfg.get('personality', 'NATIVE'))
    board = normalize_csv(build_cfg.get('board', ''))

    display_cfg = build_cfg.get("display", {})
    gui_enabled = display_cfg.get("enabled", build_cfg.get("gui", False))
    display_class = display_cfg.get("class", "none" if not gui_enabled else "generic")

    gui = 'ON' if gui_enabled else 'OFF'

    check_and_clean_stale_cache(preset)

    cmd = [
        'cmake', f'--preset={preset}',
        f'-DBHARAT_ARCH_FAMILY={arch.upper()}',
        f'-DBHARAT_DEVICE_PROFILE={profile}',
        f'-DBHARAT_PERSONALITY_PROFILE={personality}',
        f'-DBHARAT_TARGET_BOARD={board.lower()}',
        f'-DBHARAT_BOOT_GUI={gui}'
    ]
    run_command(cmd)

def do_build(build_cfg, target_name=None):
    preset = build_cfg.get('preset')
    run_command(['cmake', '--build', f'--preset={preset}'])

    if target_name:
        # Emit run_manifest.json
        target = get_target(target_name)
        validate_target(target)

        build_dir = os.path.join('build', preset)
        target_build_dir = os.path.join('build', target_name)
        os.makedirs(target_build_dir, exist_ok=True)
        manifest_path = os.path.join(target_build_dir, 'run_manifest.json')

        # Build the manifest structure
        exec_class = target.get('execution_class')
        manifest = {
            'target_name': target_name,
            'runner_type': exec_class,
            'arch': target.get('arch'),
            'profile': target.get('profile'),
            'machine_cfg': {
                'machine': target.get('machine')
            },
            'artifacts': {
                'kernel': os.path.join(build_dir, 'kernel', 'kernel.elf')
            },
            'device_list': target.get('device_contracts', {}).get('bus', []),
            'debug_config': target.get('debug_contract', {})
        }

        # Handling special boot entries
        entry = target.get('boot_contract', {}).get('entry')
        if entry == 'firmware_image':
            manifest['artifacts']['firmware'] = os.path.join(build_dir, 'firmware.bin')
        elif entry == 'memory_blob':
            manifest['artifacts']['memory_blob'] = os.path.join(build_dir, 'memory.bin')

        # DTB handling
        if target.get('discovery_contract') == 'fdt':
            manifest['dtb_handling'] = {
                'required': target.get('boot_contract', {}).get('requires_fdt', False),
                'source': target.get('boot_contract', {}).get('fdt_source', '')
            }

        # Console & Display routing
        console_mode = target.get('console_contract', {}).get('mode', 'headless')
        manifest['serial_routing'] = {
            'requested': 'serial' in console_mode,
            'stdio': True if 'serial' in console_mode else False
        }

        manifest['display_routing'] = {
            'requested': 'display' in console_mode,
            'device': target.get('console_contract', {}).get('display'),
            'frontend': 'generic' if 'display' in console_mode else None
        }

        with open(manifest_path, 'w') as f:
            json.dump(manifest, f, indent=2)
        print(f"Emitted run manifest to {manifest_path}")

def do_test(build_cfg):
    preset = build_cfg.get('preset')
    build_dir = os.path.join('build', preset)
    if not os.path.exists(build_dir):
         print(f"Build directory {build_dir} not found. Please build first.")
         sys.exit(1)
    run_command(['ctest', '--test-dir', build_dir])

def do_clean(build_cfg):
    preset = build_cfg.get('preset')
    build_dir = os.path.join('build', preset)
    if os.path.exists(build_dir):
        run_command(['cmake', '--build', f'--preset={preset}', '--target', 'clean'])

RUN_MATRIX = {
    ("x86_64", "qemu-virt"): {"runner": "qemu-system-x86_64", "machine": "q35", "smp_supported": True},
    ("arm64", "virt"): {"runner": "qemu-system-aarch64", "machine": "virt", "cpu": "cortex-a57", "smp_supported": True},
    ("riscv64", "virt"): {"runner": "qemu-system-riscv64", "machine": "virt", "smp_supported": True},
    ("arm32", "avh-corstone310"): {"runner": "VHT_Corstone_SSE-310", "smp_supported": False},
    ("arm32", "virt"): {"runner": "qemu-system-arm", "machine": "virt", "cpu": "cortex-a15", "smp_supported": True},
    ("riscv32", "virt"): {"runner": "qemu-system-riscv32", "machine": "virt", "smp_supported": True},
}

def do_run(build_cfg, dual_serial, run_tests=False, cpus=1, target_name=None):
    preset = build_cfg.get('preset')
    build_dir = os.path.join('build', preset)

    if target_name:
        manifest_path = os.path.join('build', target_name, 'run_manifest.json')
    else:
        manifest_path = os.path.join(build_dir, 'run_manifest.json')

    if target_name and os.path.exists(manifest_path):
        with open(manifest_path, 'r') as f:
            manifest = json.load(f)

        # Push dual_serial state into the manifest for the runner
        manifest['dual_serial_requested'] = dual_serial
    else:
        # Fallback to legacy execution logic for builds without a target matrix entry
        print("Warning: Running legacy execution path (no target matrix entry found).")
        print("Hint: To fix this, add proper target matrix / manifest-backed run entries to targets/target_matrix.json.")
        arch = build_cfg.get('arch')
        board = build_cfg.get('board', '')

        display_cfg = build_cfg.get("display", {})
        gui_enabled = display_cfg.get("enabled", build_cfg.get("gui", False))
        gui = gui_enabled

        target_key = (arch, board)
        run_config = RUN_MATRIX.get(target_key)
        if not run_config:
            print(f"Unsupported run configuration for arch {arch} board {board}")
            sys.exit(1)

        if cpus > 1 and not run_config.get("smp_supported", False):
            raise Exception(f"SMP not supported on this target (arch: {arch}, board: {board})")

        kernel_path = os.path.join(build_dir, 'kernel', 'kernel.elf')
        if not os.path.exists(kernel_path):
            print(f"Kernel image {kernel_path} not found. Build it first.")
            sys.exit(1)

        # Use boot abstraction layer
        boot_config = boot.get_bootloader(arch)(kernel_path)
        run_kernel = boot_config.get("kernel_path", kernel_path)

        qemu_opts = []
        if "qemu_flags" in boot_config:
            qemu_opts.extend(boot_config["qemu_flags"])

        # If run_tests is true, force headless mode for CI to capture output
        if run_tests:
            gui = False
            # In a real environment, we'd pass kernel arguments here like `-append "test=all"`
            if arch in ['x86_64', 'arm64', 'riscv64']:
                qemu_opts.extend(['-append', 'test=all log_level=debug'])

        # Serial routing: Always send primary serial to stdio for host visibility.
        # Disable the QEMU monitor on stdio to avoid stdio backend conflicts.
        # If dual serial is requested, add a second serial sink to the QEMU window (vc).
        qemu_opts.extend(['-monitor', 'none'])
        qemu_opts.extend(['-serial', 'stdio'])
        if dual_serial:
            qemu_opts.extend(['-serial', 'vc'])

        if gui:
            # Display routing: Enable GTK UI for graphical output
            qemu_opts.extend(['-display', 'gtk'])

            if arch == 'x86_64':
                # Standard VGA for x86_64 (Multiboot provides the FB info)
                qemu_opts.extend(['-vga', 'std'])
            elif arch == 'arm64' or arch == 'riscv64':
                # virtio-gpu for ARM64/RISC-V64 (modern standard)
                qemu_opts.extend(['-device', 'virtio-gpu-pci'])
        else:
            # Headless mode: No window
            qemu_opts.extend(['-display', 'none'])

        runner = run_config.get("runner")

        # [FIX] Enhanced runner check: ensure the tool is in PATH before attempting to run
        if not check_command(runner):
            print(f"\nERROR: Runner '{runner}' not found in PATH.")
            print(f"To fix this:")
            if 'qemu-system' in runner:
                print(f"  - Install QEMU for {arch}.")
                print(f"  - On Windows: Add 'C:\\Program Files\\qemu' to your PATH.")
            elif runner == 'VHT_Corstone_SSE-310':
                print(f"  - Ensure Arm Virtual Hardware (AVH) tools are installed.")
                print(f"  - Falling back to QEMU might work if you change the board profile.")
            sys.exit(1)

        if runner == 'VHT_Corstone_SSE-310':
            cmd = [runner, '-a', run_kernel] + qemu_opts
        else:
            cmd = [runner, '-kernel', run_kernel, '-m', '512']
            if "machine" in run_config:
                cmd.extend(['-machine', run_config["machine"]])
            if "cpu" in run_config:
                cmd.extend(['-cpu', run_config["cpu"]])
            if run_config.get("smp_supported", False) and cpus > 1:
                cmd.extend(['-smp', str(cpus)])
            cmd.extend(qemu_opts)

        print(f"[RUN] Arch: {arch} | CPUs: {cpus} | Board: {board} | Runner: {runner}")
        run_command(cmd)
        return

    # Delegate to the appropriate runner plugin
    runner_type = manifest.get('runner_type')

    if runner_type == 'qemu':
        from runners import runner_qemu
        runner_qemu.run(manifest)
    elif runner_type == 'renode':
        from runners import runner_renode
        runner_renode.run(manifest)
    elif runner_type == 'real_board':
        from runners import runner_real_board
        runner_real_board.run(manifest)
    elif runner_type == 'unicorn':
        from runners import runner_unicorn
        runner_unicorn.run(manifest)
    else:
        print(f"Error: Unknown runner type '{runner_type}'")
        sys.exit(1)

def do_matrix(manifest):
    matrix_targets = [
        ("x86_64_desktop_mmu", 1),
        ("x86_64_desktop_mmu", 4),
        ("arm64_desktop_mmu", 1),
        ("arm64_desktop_mmu", 4),
        ("riscv64_desktop_mmu", 1),
        ("riscv64_desktop_mmu", 4),
        ("arm32_edge_mpu", 1),
        ("arm32_virt_mmu", 1),
        ("arm32_virt_mmu", 2),
        ("riscv32_edge_mmu_lite", 1),
    ]

    results = []

    for build_name, cpus in matrix_targets:
        print(f"\n{'='*60}")
        print(f"MATRIX RUN: {build_name} with {cpus} CPU(s)")
        print(f"{'='*60}\n")

        if build_name not in manifest.get('builds', {}):
            print(f"Warning: Build '{build_name}' not found in manifest. Skipping.")
            results.append((build_name, cpus, "SKIPPED"))
            continue

        build_cfg = manifest['builds'][build_name]

        try:
            # We check if there's a corresponding target in the matrix
            target_name = None
            try:
                get_target(build_name, silent=True)
                target_name = build_name
            except (SystemExit, KeyError):
                pass

            do_configure(build_cfg)
            do_build(build_cfg, target_name)
            do_run(build_cfg, dual_serial=False, run_tests=True, cpus=cpus, target_name=target_name)
            results.append((build_name, cpus, "PASS"))
        except SystemExit as e:
            if e.code == 0:
                results.append((build_name, cpus, "PASS"))
            else:
                results.append((build_name, cpus, "FAIL"))
        except Exception as e:
            print(f"Error during matrix run for {build_name}: {e}")
            results.append((build_name, cpus, "FAIL"))

    print("\n" + "="*60)
    print("MATRIX SUMMARY")
    print("="*60)
    print(f"{'Build Name':<30} | {'CPUs':<4} | {'Status'}")
    print("-" * 60)
    for build_name, cpus, status in results:
        print(f"{build_name:<30} | {cpus:<4} | {status}")
    print("="*60 + "\n")

def main():
    # Detect legacy flags and provide a friendly migration error
    legacy_flags = {'-arch', '-clean', '-run', '-bootgui', '-dualserial', '-profile', '-e2e', '-serialtarget', '-preset'}
    used_legacy = [arg for arg in sys.argv[1:] if arg.lower() in legacy_flags]

    # Check for invalid dual_serial flag spelling
    if '--dual_serial' in sys.argv:
        print("Error: The flag is --dual-serial (with a hyphen), not --dual_serial.")
        sys.exit(1)

    if used_legacy:
        print(f"Error: Legacy flags detected: {', '.join(used_legacy)}")
        print("\nThe build system has been unified around tools/build.py with a canonical interface.")
        print("Legacy PowerShell/shell flags (like -Arch, -Clean, -Run) are NO LONGER SUPPORTED.")
        print("\nPlease use the new canonical syntax with a build name from build_config.json:")
        print("  ./build.sh <build_name> [--run] [--clean]")
        print("  .\\build.ps1 <build_name> [--run] [--clean]")
        print("\nTo see available build configurations, run:")
        print("  ./build.sh --list")
        sys.exit(1)

    parser = argparse.ArgumentParser(
        description='Bharat-OS Build Wrapper\n\n'
                    'Examples:\n'
                    '  python tools/build.py default_dev --run\n'
                    '  python tools/build.py arm64_automobile_debug --clean --build\n'
                    '  python tools/build.py riscv64_edge_mmu_lite\n'
                    '  python tools/build.py --list',
        formatter_class=argparse.RawTextHelpFormatter
    )
    parser.add_argument('build_name', nargs='?', default='default_dev', help='Name of the build from build_config.json')
    parser.add_argument('--list', action='store_true', help='List available builds')
    parser.add_argument('--doctor', action='store_true', help='Check environment')
    parser.add_argument('--configure', action='store_true', help='Only configure the project')
    parser.add_argument('--build', action='store_true', help='Build the project')
    parser.add_argument('--run', action='store_true', help='Run the project in emulator')
    parser.add_argument('--clean', action='store_true', help='Clean the build directory')
    parser.add_argument('--test', action='store_true', help='Run tests (host)')
    parser.add_argument('--run-tests', action='store_true', help='Run user-space and kernel tests in emulator (headless)')
    parser.add_argument('--dual-serial', action='store_true', help='Use dual serial ports in emulator')
    parser.add_argument('--cpus', type=int, default=1, help='Number of CPUs for SMP run (if supported)')
    parser.add_argument('--matrix', action='store_true', help='Run the execution matrix sequentially')

    args = parser.parse_args()

    if args.doctor:
        doctor()
        return

    manifest = load_manifest()

    if args.list:
        print_list(manifest)
        return

    if args.matrix:
        do_matrix(manifest)
        return

    # Determine target_name if it exists in the new target matrix
    target_name = None
    try:
        target = get_target(args.build_name, silent=True)
        target_name = args.build_name
    except (SystemExit, KeyError, Exception):
        pass

    if args.build_name not in manifest.get('builds', {}):
        if target_name:
            # We found a new target matrix target but no build config. Create a dummy build_cfg to proceed.
            # In a real system, you'd map the build config dynamically based on target.
            print(f"Warning: No explicit build config found for {args.build_name}, proceeding with dynamic settings.")
            # Map standard profiles to existing presets
            profile_to_preset = {
                'small': 'edge',
                'desktop': 'edge', # Desktop is mapped to edge in legacy config for non-x86
                'tiny': 'edge' # map to edge for now since tiny-mpu-debug lacks arch prefix
            }
            preset_suffix = profile_to_preset.get(target.get('profile'), 'edge')
            if target.get('arch') == 'x86_64':
                 preset_suffix = 'dev' # x86_64 uses -dev or -debug

            build_cfg = {
                'preset': f"{target.get('arch')}-{preset_suffix}",
                'arch': target.get('arch'),
                'board': target.get('machine'),
                'gui': 'display' in target.get('console_contract', {}).get('mode', 'headless')
            }
        else:
            print(f"Error: Build/Target '{args.build_name}' not found in manifest or target matrix.")
            print_list(manifest)
            sys.exit(1)
    else:
        build_cfg = manifest['builds'][args.build_name]

    # Default action if no specific action provided
    if not (args.configure or args.build or args.run or args.clean or args.test or args.run_tests):
        args.configure = True
        args.build = True
        if build_cfg.get('run', False):
            args.run = True

    if args.clean:
        do_clean(build_cfg)

    if args.configure:
        do_configure(build_cfg)

    if args.build:
        do_build(build_cfg, target_name)

    if args.test:
        do_test(build_cfg)

    if args.run or args.run_tests:
        do_run(build_cfg, args.dual_serial, args.run_tests, args.cpus, target_name)

if __name__ == '__main__':
    main()
