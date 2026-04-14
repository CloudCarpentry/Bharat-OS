import argparse
import json
import os
import subprocess
import sys
import boot

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
    elif arch == 'arm32':
        return check_command('VHT_Corstone_SSE-310')
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
    runners = ['x86_64', 'arm64', 'riscv64', 'arm32']
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
    gui = 'ON' if build_cfg.get('gui') else 'OFF'

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

def do_build(build_cfg):
    preset = build_cfg.get('preset')
    run_command(['cmake', '--build', f'--preset={preset}'])

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

def do_run(build_cfg, dual_serial, run_tests=False):
    arch = build_cfg.get('arch')
    board = build_cfg.get('board', '')
    gui = build_cfg.get('gui', False)
    preset = build_cfg.get('preset')

    kernel_path = os.path.join('build', preset, 'kernel', 'kernel.elf')
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

    if gui:
        if dual_serial:
            qemu_opts.extend(['-serial', 'stdio', '-serial', 'vc'])
        else:
            qemu_opts.extend(['-serial', 'vc'])

        if arch == 'x86_64':
            qemu_opts.extend(['-vga', 'std'])
        elif arch == 'arm64':
            qemu_opts.extend(['-device', 'virtio-gpu-pci'])
        elif arch == 'riscv64':
            qemu_opts.extend(['-device', 'virtio-gpu-device', '-device', 'ramfb'])
    else:
        qemu_opts.extend(['-serial', 'stdio', '-display', 'none'])

    if arch == 'x86_64':
        cmd = ['qemu-system-x86_64', '-kernel', run_kernel, '-m', '512'] + qemu_opts
    elif arch == 'arm64':
        cmd = ['qemu-system-aarch64', '-kernel', run_kernel, '-m', '512'] + qemu_opts
    elif arch == 'riscv64':
        cmd = ['qemu-system-riscv64', '-kernel', run_kernel, '-m', '512'] + qemu_opts
    elif arch == 'arm32' and board == 'avh-corstone310':
        cmd = ['VHT_Corstone_SSE-310', '-a', run_kernel] + qemu_opts
    else:
        print(f"Unsupported run configuration for arch {arch} board {board}")
        sys.exit(1)

    run_command(cmd)

def main():
    # Detect legacy flags and provide a friendly migration error
    legacy_flags = {'-arch', '-clean', '-run', '-bootgui', '-dualserial', '-profile', '-e2e', '-serialtarget', '-preset'}
    used_legacy = [arg for arg in sys.argv[1:] if arg.lower() in legacy_flags]
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

    args = parser.parse_args()

    if args.doctor:
        doctor()
        return

    manifest = load_manifest()

    if args.list:
        print_list(manifest)
        return

    if args.build_name not in manifest.get('builds', {}):
        print(f"Error: Build '{args.build_name}' not found in manifest.")
        print_list(manifest)
        sys.exit(1)

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
        do_build(build_cfg)

    if args.test:
        do_test(build_cfg)

    if args.run or args.run_tests:
        do_run(build_cfg, args.dual_serial, args.run_tests)

if __name__ == '__main__':
    main()
