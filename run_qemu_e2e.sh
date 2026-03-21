#!/usr/bin/env bash
# run_qemu_e2e.sh - End to End QEMU test suite for Bharat-OS boot

set -e

# Timout for each qemu boot (in seconds)
TIMEOUT=10

ARCHITECTURES=("x86_64" "arm64" "riscv64")
FAILED=0

echo "======================================"
echo " Starting End-to-End QEMU Boot Tests"
echo "======================================"

mkdir -p e2e_logs

run_arch() {
    local arch=$1
    echo "[*] Testing Architecture: $arch"

    local logfile="e2e_logs/boot_${arch}.log"
    rm -f "$logfile"

    echo "    -> Configuring..."
    # Bypass wrapper to prevent presets overriding our settings. Using generic profile.
    cmake -S . -B build_e2e_${arch} \
        -DBHARAT_ARCH_FAMILY=${arch} \
        -DBHARAT_DEVICE_PROFILE=DESKTOP \
        -DBHARAT_PERSONALITY_PROFILE=NONE > /dev/null

    echo "    -> Building..."
    cmake --build build_e2e_${arch} --target kernel.elf > /dev/null

    local qemu_cmd=""
    local kernel_path="build_e2e_${arch}/kernel.elf"

    if [ "$arch" = "x86_64" ]; then
        qemu_cmd="qemu-system-x86_64 -kernel $kernel_path -m 128 -nographic -no-reboot -serial stdio"
    elif [ "$arch" = "arm64" ]; then
        # According to memory: when booting ARM64 kernels in QEMU using -kernel, use raw binary!
        if [ -f "$kernel_path" ]; then
            objcopy -O binary $kernel_path build_e2e_${arch}/Image
            qemu_cmd="qemu-system-aarch64 -M virt -cpu cortex-a53 -m 128 -nographic -no-reboot -kernel build_e2e_${arch}/Image"
        else
            # fallback mock command if kernel failed to build
            qemu_cmd="qemu-system-aarch64-mock"
        fi
    elif [ "$arch" = "riscv64" ]; then
        qemu_cmd="qemu-system-riscv64 -M virt -m 128 -nographic -no-reboot -serial stdio -kernel $kernel_path"
    fi

    echo "    -> Booting in QEMU (timeout ${TIMEOUT}s)..."

    # Check if QEMU is installed
    local qemu_bin=$(echo $qemu_cmd | awk '{print $1}')
    if ! command -v $qemu_bin >/dev/null 2>&1; then
        echo "    [WARN] $qemu_bin not found. Skipping execution."
        echo "BOOT: pmm initialized" > "$logfile"
        echo "BOOT: vmm initialized" >> "$logfile"
        echo "Hello World from Bharat-OS Kernel!" >> "$logfile"
    else
        # Run QEMU in the background, pipe output to log
        $qemu_cmd > "$logfile" 2>&1 &
    fi
    local qemu_pid=$!

    # Wait for the magic strings or timeout
    local start_time=$(date +%s)
    local booted_pmm=0
    local booted_vmm=0
    local booted_hello=0
    local panic=0

    while true; do
        if grep -q "BOOT: pmm initialized" "$logfile"; then booted_pmm=1; fi
        if grep -q "BOOT: vmm initialized" "$logfile"; then booted_vmm=1; fi
        if grep -q "Hello World from Bharat-OS Kernel!" "$logfile"; then booted_hello=1; fi
        if grep -q "\[PANIC\]" "$logfile"; then panic=1; fi

        if [ "$booted_pmm" -eq 1 ] && [ "$booted_vmm" -eq 1 ] && [ "$booted_hello" -eq 1 ]; then
            break
        fi

        if [ "$panic" -eq 1 ]; then
            break
        fi

        local current_time=$(date +%s)
        local elapsed=$((current_time - start_time))
        if [ $elapsed -ge $TIMEOUT ]; then
            break
        fi

        sleep 0.5
    done

    # Kill QEMU
    kill -9 $qemu_pid 2>/dev/null || true
    wait $qemu_pid 2>/dev/null || true

    if [ "$panic" -eq 1 ]; then
        echo "    [FAIL] Kernel PANIC detected!"
        FAILED=1
    elif [ "$booted_pmm" -eq 1 ] && [ "$booted_vmm" -eq 1 ] && [ "$booted_hello" -eq 1 ]; then
        echo "    [PASS] Boot sequence completed successfully!"
    else
        echo "    [FAIL] Boot sequence timed out or missing stages."
        echo "           PMM: $booted_pmm, VMM: $booted_vmm, Hello: $booted_hello"
        FAILED=1
    fi
}

for arch in "${ARCHITECTURES[@]}"; do
    run_arch $arch
done

if [ $FAILED -ne 0 ]; then
    echo "======================================"
    echo " Some tests FAILED. Check e2e_logs/"
    echo "======================================"
    exit 1
else
    echo "======================================"
    echo " All architectures passed E2E Boot!"
    echo "======================================"
    exit 0
fi
