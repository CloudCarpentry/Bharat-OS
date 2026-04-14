import sys

def run(manifest):
    print("[Unicorn Runner] Validating manifest...")

    # Reject: UART/display/network requirements
    devices = manifest.get('device_list', [])
    if devices:
        print("Error: Unicorn Runner rejects device requirements (UART/display/network).")
        sys.exit(1)

    serial = manifest.get('serial_routing', {})
    if serial.get('requested'):
        print("Error: Unicorn Runner rejects serial routing.")
        sys.exit(1)

    artifacts = manifest.get('artifacts', {})
    if not artifacts.get('memory_blob'):
        print("Error: Unicorn Runner requires memory blob artifact.")
        sys.exit(1)

    print("[Unicorn Runner] Generating CPU harness execution plan...")

    plan = f"""
    CPU Arch: {manifest.get('arch')}
    Memory Blob: {artifacts.get('memory_blob')}
    Mapping Strategy: Flat CPU Memory
    """

    print(f"[Unicorn Runner] Harness Plan:\n{plan}")
    print("[Unicorn Runner] Harness execution successful (stub).")
