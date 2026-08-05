#!/usr/bin/env bash
#
# Bharat-OS Codex Cloud environment setup
#
# Supports:
#   x86_64
#   arm64
#   arm32
#   riscv64
#   riscv32
#
# Usage:
#   bash tools/codex/setup_and_test_qemu_5arch.sh install
#   bash tools/codex/setup_and_test_qemu_5arch.sh verify
#   bash tools/codex/setup_and_test_qemu_5arch.sh five
#   bash tools/codex/setup_and_test_qemu_5arch.sh matrix
#
# Default:
#   bash tools/codex/setup_and_test_qemu_5arch.sh
#   Equivalent to: five
#
# Environment overrides:
#   BHARAT_SKIP_APT=1
#   BHARAT_OS_ROOT=/workspace/Bharat-OS
#   QEMU_SMOKE_TIMEOUT_SECS=300
#

set -Eeuo pipefail

MODE="${1:-five}"
TIMEOUT_SECS="${QEMU_SMOKE_TIMEOUT_SECS:-300}"
SKIP_APT="${BHARAT_SKIP_APT:-0}"

log() {
    printf '\n\033[1;36m[codex-qemu]\033[0m %s\n' "$*"
}

warn() {
    printf '\n\033[1;33m[codex-qemu][WARN]\033[0m %s\n' "$*" >&2
}

die() {
    printf '\n\033[1;31m[codex-qemu][ERROR]\033[0m %s\n' "$*" >&2
    exit 1
}

on_error() {
    local rc=$?
    printf '\n\033[1;31m[codex-qemu]\033[0m Failed at line %s, exit code %s\n' \
        "${BASH_LINENO[0]}" "$rc" >&2
    exit "$rc"
}

trap on_error ERR

case "$MODE" in
    install|verify|five|matrix)
        ;;
    *)
        die "Unknown mode '$MODE'. Use: install, verify, five, or matrix."
        ;;
esac

# ---------------------------------------------------------------------------
# Privilege handling
# ---------------------------------------------------------------------------

if [[ "$(id -u)" -eq 0 ]]; then
    SUDO=()
elif command -v sudo >/dev/null 2>&1; then
    SUDO=(sudo)
else
    die "Root privileges or sudo are required to install host packages."
fi

# ---------------------------------------------------------------------------
# Package installation
# ---------------------------------------------------------------------------

install_packages() {
    if [[ "$SKIP_APT" == "1" ]]; then
        warn "Skipping apt installation because BHARAT_SKIP_APT=1."
        return
    fi

    command -v apt-get >/dev/null 2>&1 ||
        die "This script currently supports Ubuntu/Debian Codex environments."

    log "Updating apt package metadata"
    "${SUDO[@]}" apt-get update -qq

    log "Installing compiler, packaging, Python, and QEMU dependencies"

    DEBIAN_FRONTEND=noninteractive "${SUDO[@]}" apt-get install -y \
        --no-install-recommends \
        ca-certificates \
        git \
        curl \
        file \
        jq \
        rsync \
        pkg-config \
        build-essential \
        cmake \
        ninja-build \
        clang \
        lld \
        llvm \
        python3 \
        python3-pip \
        python3-venv \
        python3-dev \
        python3-yaml \
        python3-jsonschema \
        qemu-system-x86 \
        qemu-system-arm \
        qemu-system-misc \
        qemu-utils \
        device-tree-compiler \
        grub-common \
        grub-pc-bin \
        xorriso \
        mtools \
        dosfstools \
        cpio \
        gdb-multiarch

    # Optional packages vary between Ubuntu/Debian releases.
    local optional_packages=(
        opensbi
        grub2-common
        ovmf
        binutils-aarch64-linux-gnu
        binutils-arm-linux-gnueabihf
        binutils-riscv64-linux-gnu
    )

    local package
    for package in "${optional_packages[@]}"; do
        if apt-cache show "$package" >/dev/null 2>&1; then
            log "Installing optional package: $package"
            DEBIAN_FRONTEND=noninteractive "${SUDO[@]}" apt-get install -y \
                --no-install-recommends "$package"
        else
            warn "Optional package is unavailable: $package"
        fi
    done
}

# ---------------------------------------------------------------------------
# Repository detection
# ---------------------------------------------------------------------------

find_repository_root() {
    if [[ -n "${BHARAT_OS_ROOT:-}" ]]; then
        cd "$BHARAT_OS_ROOT"
    elif git rev-parse --show-toplevel >/dev/null 2>&1; then
        cd "$(git rev-parse --show-toplevel)"
    elif [[ -f "./tools/build.py" && -d "./delivery/targets/qemu" ]]; then
        cd .
    else
        die "Run this script inside the Bharat-OS checkout or set BHARAT_OS_ROOT."
    fi

    [[ -f tools/build.py ]] ||
        die "tools/build.py was not found in $(pwd)."

    [[ -d delivery/targets/qemu ]] ||
        die "delivery/targets/qemu was not found in $(pwd)."

    ROOT_DIR="$(pwd)"
    export ROOT_DIR

    log "Repository root: $ROOT_DIR"

    if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
        local branch
        local commit

        branch="$(git branch --show-current 2>/dev/null || true)"
        commit="$(git rev-parse --short HEAD 2>/dev/null || true)"

        log "Git branch: ${branch:-detached}"
        log "Git commit: ${commit:-unknown}"

        if [[ "$branch" != "developer" ]]; then
            warn "Current branch is '${branch:-detached}', not 'developer'."
            warn "The script will not switch branches or discard local changes."
        fi
    fi
}

# ---------------------------------------------------------------------------
# Python environment
# ---------------------------------------------------------------------------

setup_python_environment() {
    local venv_dir="$ROOT_DIR/.venv-codex"

    if [[ ! -x "$venv_dir/bin/python3" ]]; then
        log "Creating Python virtual environment: $venv_dir"
        python3 -m venv "$venv_dir"
    fi

    # shellcheck disable=SC1091
    source "$venv_dir/bin/activate"

    log "Installing Python build dependencies"
    python3 -m pip install --quiet --upgrade \
        pip \
        setuptools \
        wheel

    python3 -m pip install --quiet \
        pyyaml \
        jsonschema

    export PYTHON_BIN="$venv_dir/bin/python3"

    "$PYTHON_BIN" - <<'PY'
import sys
import yaml
import jsonschema

print(f"Python: {sys.version.split()[0]}")
print(f"PyYAML: {yaml.__version__}")
print(f"jsonschema: {jsonschema.__version__}")
PY
}

# ---------------------------------------------------------------------------
# Tool verification
# ---------------------------------------------------------------------------

require_command() {
    local command_name="$1"

    if ! command -v "$command_name" >/dev/null 2>&1; then
        die "Required command is missing: $command_name"
    fi
}

verify_environment() {
    log "Verifying compiler and build tools"

    local commands=(
        git
        cmake
        ninja
        clang
        ld.lld
        llvm-ar
        llvm-objcopy
        python3
        timeout
        dtc
        xorriso
        grub-mkrescue
    )

    local command_name
    for command_name in "${commands[@]}"; do
        require_command "$command_name"
        printf '  %-28s %s\n' "$command_name" "$(command -v "$command_name")"
    done

    log "Verifying all five QEMU architecture binaries"

    local qemu_commands=(
        qemu-system-x86_64
        qemu-system-aarch64
        qemu-system-arm
        qemu-system-riscv64
        qemu-system-riscv32
    )

    for command_name in "${qemu_commands[@]}"; do
        require_command "$command_name"

        printf '  %-28s %s\n' \
            "$command_name" \
            "$("$command_name" --version | head -n 1)"
    done

    log "QEMU architecture verification passed"
}

# ---------------------------------------------------------------------------
# Target validation
# ---------------------------------------------------------------------------

validate_target() {
    local target="$1"

    [[ -f "$target" ]] ||
        die "Target YAML does not exist: $target"
}

# ---------------------------------------------------------------------------
# Five-architecture smoke tests
# ---------------------------------------------------------------------------

run_five_architectures() {
    local targets=(
        "x86_64|delivery/targets/qemu/x86_64_desktop_headless.yaml"
        "arm64|delivery/targets/qemu/arm64_desktop_headless.yaml"
        "riscv64|delivery/targets/qemu/riscv64_desktop_headless.yaml"
        "arm32|delivery/targets/qemu/arm32_mmu_lite_headless.yaml"
        "riscv32|delivery/targets/qemu/riscv32_mmu_lite_headless.yaml"
    )

    local timestamp
    timestamp="$(date -u +%Y%m%dT%H%M%SZ)"

    local log_dir="$ROOT_DIR/build/codex-qemu-logs/$timestamp"
    mkdir -p "$log_dir"

    local -a result_arches=()
    local -a result_codes=()
    local pass_count=0
    local fail_count=0

    log "Running one canonical headless smoke target per architecture"
    log "Per-target timeout: ${TIMEOUT_SECS} seconds"
    log "Logs: $log_dir"

    local entry
    local arch
    local target
    local rc

    for entry in "${targets[@]}"; do
        arch="${entry%%|*}"
        target="${entry#*|}"

        validate_target "$target"

        printf '\n'
        printf '%s\n' "================================================================"
        printf 'Architecture : %s\n' "$arch"
        printf 'Target       : %s\n' "$target"
        printf '%s\n' "================================================================"

        set +e

        timeout \
            --kill-after=15s \
            "${TIMEOUT_SECS}s" \
            "$PYTHON_BIN" tools/build.py all \
                --target-yaml "$target" \
                --smoke \
            2>&1 | tee "$log_dir/${arch}.log"

        rc=${PIPESTATUS[0]}

        set -e

        result_arches+=("$arch")
        result_codes+=("$rc")

        if [[ "$rc" -eq 0 ]]; then
            ((pass_count += 1))
            printf '\n[codex-qemu] PASS: %s\n' "$arch"
        else
            ((fail_count += 1))
            printf '\n[codex-qemu] FAIL: %s, exit code %s\n' "$arch" "$rc"

            if [[ "$rc" -eq 124 || "$rc" -eq 137 || "$rc" -eq 143 ]]; then
                printf '[codex-qemu] Target may have exceeded its timeout.\n'
            fi
        fi
    done

    printf '\n'
    printf '%s\n' "================================================================"
    printf 'BHARAT-OS FIVE-ARCHITECTURE SUMMARY\n'
    printf '%s\n' "================================================================"
    printf '%-12s %-10s %-10s\n' "ARCH" "RESULT" "EXIT CODE"
    printf '%-12s %-10s %-10s\n' "------------" "----------" "----------"

    local i
    local result

    for i in "${!result_arches[@]}"; do
        rc="${result_codes[$i]}"

        if [[ "$rc" -eq 0 ]]; then
            result="PASS"
        else
            result="FAIL"
        fi

        printf '%-12s %-10s %-10s\n' \
            "${result_arches[$i]}" \
            "$result" \
            "$rc"
    done

    printf '\nPassed: %s\n' "$pass_count"
    printf 'Failed: %s\n' "$fail_count"
    printf 'Logs  : %s\n' "$log_dir"

    if [[ "$fail_count" -ne 0 ]]; then
        return 1
    fi
}

# ---------------------------------------------------------------------------
# Complete GP, RT, and SMP matrix
# ---------------------------------------------------------------------------

run_complete_matrix() {
    local matrix_script="tools/ci/run_qemu_headless_matrix.sh"

    [[ -f "$matrix_script" ]] ||
        die "Canonical matrix script was not found: $matrix_script"

    local timestamp
    timestamp="$(date -u +%Y%m%dT%H%M%SZ)"

    local log_dir="$ROOT_DIR/build/codex-qemu-logs/$timestamp"
    mkdir -p "$log_dir"

    log "Running the complete canonical QEMU matrix"
    log "This includes GP, RT, MPU/MMU-Lite, and 64-bit SMP lanes."
    log "Log: $log_dir/full-matrix.log"

    set +e

    QEMU_MATRIX_TIMEOUT_SECS="$TIMEOUT_SECS" \
    PYTHON_BIN="$PYTHON_BIN" \
        bash "$matrix_script" \
        2>&1 | tee "$log_dir/full-matrix.log"

    local rc=${PIPESTATUS[0]}

    set -e

    if [[ "$rc" -ne 0 ]]; then
        warn "The complete QEMU matrix failed with exit code $rc."
        return "$rc"
    fi

    log "Complete QEMU matrix passed"
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

main() {
    install_packages

    if [[ "$MODE" == "install" ]]; then
        log "Installation completed"
        verify_environment
        return
    fi

    find_repository_root
    setup_python_environment
    verify_environment

    case "$MODE" in
        verify)
            log "Environment verification completed"
            ;;
        five)
            run_five_architectures
            ;;
        matrix)
            run_complete_matrix
            ;;
    esac
}

main "$@"