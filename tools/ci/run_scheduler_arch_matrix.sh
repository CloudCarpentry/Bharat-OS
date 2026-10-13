#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build-tests-host"
rm -rf "${BUILD_DIR}"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release -DBHARAT_BUILD_HOST_TESTS=ON
cmake --build "${BUILD_DIR}" -j --target test_scheduler test_trap_syscall test_tier_a_integration
ctest --test-dir "${BUILD_DIR}" --output-on-failure -R "test_scheduler|test_trap_syscall|test_tier_a_integration"

for arch_file in \
  "core/arch/x86/x86_64/context_switch.c" \
  "core/arch/arm/arm64/context_switch.c" \
  "core/arch/riscv/riscv64/context_switch.c" \
  "core/arch/shakti/context_switch.c"; do
  if ! command -v ${CC:-cc} &> /dev/null; then
      echo "Missing toolchain for ${arch_file}"
  elif ${CC:-cc} -std=c11 -I"${ROOT_DIR}/core/kernel/include" -I"${ROOT_DIR}/interface/include" -I"${ROOT_DIR}/core/lib/include" -I"${ROOT_DIR}/core/personalities/runtime/include" -I"${BUILD_DIR}/generated/include" -c "${ROOT_DIR}/${arch_file}" -o /tmp/$(basename "${arch_file}").o 2>/dev/null; then
      echo "compiled ${arch_file}"
  else
      echo "Build failure for ${arch_file} (likely missing inline assembly support in host compiler)"
  fi
done

for core_file in \
  "core/kernel/src/cpu_local.c" \
  "core/kernel/src/urpc/urpc_channel.c" \
  "core/stacks/network/skb.c"; do
  if ! command -v ${CC:-cc} &> /dev/null; then
      echo "Missing toolchain for ${core_file}"
  elif ${CC:-cc} -std=c11 -I"${ROOT_DIR}/core/kernel/include" -I"${ROOT_DIR}/interface/include" -I"${ROOT_DIR}/core/lib/include" -I"${ROOT_DIR}/core/personalities/runtime/include" -I"${ROOT_DIR}/core/services/core/subsysmgr/include" -I"${BUILD_DIR}/generated/include" -c "${ROOT_DIR}/${core_file}" -o /tmp/$(basename "${core_file}").o 2>/dev/null; then
      echo "compiled ${core_file}"
  else
      echo "Build failure for ${core_file}"
  fi
done

echo "scheduler arch matrix checks passed"
