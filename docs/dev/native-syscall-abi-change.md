# Native syscall ABI verification and intentional changes

`interface/contracts/abi/native_syscalls.json` is the sole Native syscall ABI
source of truth. `interface/contracts/abi/native_syscalls.lock.json` freezes the
existing number-to-symbol assignments, ABI-relevant metadata, manifest count,
and deterministic hashes of the three build-generated outputs:

- `numbers.h`, consumed through `bh_syscall_numbers.h`;
- `table.def`, consumed through the public table wrapper; and
- `native_syscall_table.inc`, consumed by the Native personality dispatch table.

The generated files remain build-tree artifacts and must not be copied into the
source tree. The common `bh_syscall_gate()` remains the routing authority: it
selects the Native, Linux, or Android personality table before performing
metadata-driven dispatch. ABI verification does not add compatibility calls or
change personality behavior.

## Reproducibility gate

Run from the repository root:

```bash
python3 tools/abi/syscall_abi.py --check
python3 -m unittest tools.abi.test_syscall_abi
```

The check validates schema and semantic constraints, rejects duplicate numbers,
compares every locked syscall by symbol (including its number and ABI metadata),
requires the manifest/table count to equal the lock, generates all outputs twice
in temporary directories, and compares their SHA-256 values with the committed
lock. It therefore fails for a renumber, an unlocked addition, stale generated
expectations, metadata count drift, or nondeterministic generation.

## Intentional ABI-change procedure

1. Obtain ABI compatibility review before editing the manifest. Existing Native
   syscall numbers, symbols, argument layouts, capability requirements, and
   traits must not change under the append-only policy.
2. Add the new Native syscall to `native_syscalls.json` with an unused fixed
   number and complete capability/usercopy metadata. Do not edit generated files.
3. Confirm the ordinary check fails because the addition is not locked.
4. After approval, intentionally refresh the lock:

   ```bash
   python3 tools/abi/syscall_abi.py --update-lock
   ```

5. Review both manifest and lock diffs, then run the reproducibility gate, ABI
   unit tests, repository linters, and required target/QEMU validation matrix.

`--update-lock` is never a repair for an unexplained failure. A generated hash
change without an intentional generator change must be investigated and
reverted. Linux and Android syscall additions belong to their compatibility
contracts and are outside this Native ABI procedure.
