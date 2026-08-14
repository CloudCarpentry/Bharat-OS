1. **Fix stale/fragile include paths.**
   - In `core/arch/`, update relative includes like `#include "../../kernel/include/hal/hal_pt.h"` to canonical paths like `#include <hal/hal_pt.h>`.
   - Update includes in `core/kernel/include/hal/` to be canonical as well.
   - Update any `CMakeLists.txt` in `core/arch/` to remove `-I../../../kernel/include` and use appropriate relative paths if necessary.

2. **Remove CI "|| true".**
   - In `.github/workflows/gate1-pr-fast.yml`, find the `build-tidy` step and remove `|| true` so that compile failures break the build.

3. **Require configure + compile:**
   - Ensure the `quality/ci/pr-fast-targets.yaml` file tests the architectures `x86_64`, `arm64`, `arm32`, `riscv64`, `riscv32`.

4. **Smoke boot:**
   - In `.github/workflows/gate1-pr-fast.yml`, update the `smoke-qemu` step to only use `--smoke` on `x86_64`, `arm64`, and `riscv64`.

5. **Compile-only initially where emulator support is weaker:**
   - In the same `smoke-qemu` step, use a conditional to run a `build` rather than a full `smoke` test for `arm32` and `riscv32`.

6. **Add lint for core/arch/hal must not exist.**
   - Ensure `tools/lint/check_placement.py` or similar enforces that `core/arch/hal` does not exist. The current check does this: `if os.path.exists(arch_hal_dir):` ... `report_violation`.

7. **Add lint for canonical include contracts.**
   - Create `tools/lint/check_canonical_includes.py` to assert that `#include "../../kernel/include/...` and similar are not used in `core/arch/`.
   - Add this script to `.github/workflows/gate1-pr-fast.yml`.

8. **Check no architecture target references removed paths.**
   - Create `tools/lint/check_architecture_references.py` to ensure `kernel/include` and similar paths are not used in `core/arch/`.
   - Add this script to `.github/workflows/gate1-pr-fast.yml`.

9. **Keep all architecture build warnings at zero.**
   - The modified CI will now fail on compiler warnings for the kernel.

10. **Pre-commit checks**
   - Ensure proper testing, verification, review, and reflection are done by calling `pre_commit_instructions`.
