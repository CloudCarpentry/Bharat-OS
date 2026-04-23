import sys

def run(manifest):
    print("[Renode Runner] Validating manifest...")

    # Reject missing peripheral description
    if not manifest.get('device_list'):
        print("Error: Renode Runner rejects missing peripheral description.")
        sys.exit(1)

    # Reject desktop PCI assumptions
    devices = manifest.get('device_list', [])
    if any('pci' in d for d in devices):
        print("Error: Renode Runner rejects desktop PCI assumptions.")
        sys.exit(1)

    artifacts = manifest.get('artifacts', {})
    if not artifacts.get('kernel'):
        print("Error: Renode Runner requires kernel artifact.")
        sys.exit(1)

    # In a real runner, we would map peripherals via .repl file
    print("[Renode Runner] Generating Renode script...")

    script = f"""
    mach create
    machine LoadPlatformDescription @platforms/boards/{manifest.get('machine_cfg', {}).get('machine')}.repl
    sysbus LoadELF @{artifacts.get('kernel')}
    """

    print(f"[Renode Runner] Generated Execution Plan:\n{script}")
    print("[Renode Runner] Execution successful (stub).")
