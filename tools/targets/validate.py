import sys

def validate_target(target):
    name = target.get('name', 'unknown')
    execution_class = target.get('execution_class')
    discovery_contract = target.get('discovery_contract')
    console_contract = target.get('console_contract', {})
    console_mode = console_contract.get('mode', 'headless')
    boot_contract = target.get('boot_contract', {})
    device_contracts = target.get('device_contracts', {})

    # 1. FAIL if: display=true but no display device
    if 'display' in console_mode:
        if 'display' not in console_contract:
            print(f"Error: Target {name} requests display mode '{console_mode}' but has no display device contract.")
            sys.exit(1)

    # 2. FAIL if: discovery=fdt but no dtb source
    if discovery_contract == 'fdt':
        if not boot_contract.get('requires_fdt') or not boot_contract.get('fdt_source'):
            print(f"Error: Target {name} requests discovery=fdt but boot_contract lacks requires_fdt=True or fdt_source.")
            sys.exit(1)

    # 3. FAIL if: execution=unicorn + any device requirement
    if execution_class == 'unicorn':
        if console_mode != 'headless' or target.get('device_contracts'):
            print(f"Error: Target {name} uses execution_class=unicorn but declares device/console contracts.")
            sys.exit(1)

    # 4. FAIL if: real_board without firmware/debug config
    if execution_class == 'real_board':
        if not target.get('debug_contract'):
            print(f"Error: Target {name} uses execution_class=real_board but lacks debug_contract.")
            sys.exit(1)
        if boot_contract.get('entry') != 'firmware_image':
             print(f"Error: Target {name} uses execution_class=real_board but entry is not firmware_image.")
             sys.exit(1)

    # 5. FAIL if: serial required but no UART
    if 'serial' in console_mode:
        if 'primary_uart' not in console_contract:
             print(f"Error: Target {name} requests serial in console mode but has no primary_uart contract.")
             sys.exit(1)

    return True

def validate_manifest(manifest):
    # This validates the run manifest right before runners consume it
    runner_type = manifest.get('runner_type')
    if not runner_type:
        print("Error: Run manifest missing runner_type.")
        sys.exit(1)

    # Specific runner validation can also be delegated to the runners themselves,
    # but basic structural checks could go here.
    return True
