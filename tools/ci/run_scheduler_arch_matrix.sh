#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build-tests-host"
rm -rf "${BUILD_DIR}"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release -DBHARAT_BUILD_HOST_TESTS=ON
cmake --build "${BUILD_DIR}" -j --target test_scheduler test_trap_syscall test_tier_a_integration
ctest --test-dir "${BUILD_DIR}" --output-on-failure -R "test_scheduler|test_trap_syscall|test_tier_a_integration"

for arch_file in \
  "core/kernel/src/arch/x86_64/context_switch.c" \
  "core/kernel/src/arch/arm64/context_switch.c" \
  "core/kernel/src/arch/riscv64/context_switch.c" \
  "core/kernel/src/arch/shakti/context_switch.c"; do
  cc -std=c11 -I"${ROOT_DIR}/core/kernel/include" -c "${ROOT_DIR}/${arch_file}" -o /tmp/$(basename "${arch_file}").o
  echo "compiled ${arch_file}"
done

for core_file in \
  "core/kernel/src/cpu_local.c" \
  "core/kernel/src/urpc/urpc_channel.c" \
  "core/stacks/network/skb.c"; do
  cc -std=c11 -I"${ROOT_DIR}/core/kernel/include" -I"${ROOT_DIR}/core/services/core/subsysmgr/include" -c "${ROOT_DIR}/${core_file}" -o /tmp/$(basename "${core_file}").o
  echo "compiled ${core_file}"
done

echo "scheduler arch matrix checks passed"
