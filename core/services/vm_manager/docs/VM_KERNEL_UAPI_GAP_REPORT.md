# VM Kernel UAPI Gap Report

## A. Verified UAPI Inventory

For each required VM authority operation, the current state of the UAPI is analyzed below:

1.  **Create VM Space (`space_create`)**
    *   **Existing UAPI:** None.
    *   **Actual implemented kernel handler:** `aspace_create` or `vm_space_create` inside kernel `mm/vm/aspace/vm_space.c`.
    *   **Target VM-space identity:** Missing.
    *   **Required capability:** `CAP_TYPE_VM_SPACE` with `BH_CAP_RIGHT_WRITE` / `BH_CAP_RIGHT_RESOURCE_ALLOC` (conceptual).
    *   **Supported memory profiles:** `MEM_PROFILE_MMU_BASIC`, `MEM_PROFILE_MMU_FULL`, `MEM_PROFILE_MPU_ONLY`.
    *   **Missing behavior:** No capability-secured userspace interface to allocate and initialize a generic VM space.
    *   **Status:** BLOCKED.

2.  **Destroy VM Space (`space_destroy`)**
    *   **Existing UAPI:** None.
    *   **Actual implemented kernel handler:** `vm_space_destroy` / `aspace_destroy`.
    *   **Target VM-space identity:** Missing.
    *   **Required capability:** `CAP_TYPE_VM_SPACE` with `BH_CAP_RIGHT_DESTROY`.
    *   **Supported memory profiles:** All.
    *   **Missing behavior:** No explicit syscall for destroying a capability-backed VM space.
    *   **Status:** BLOCKED.

3.  **Map Region (`map`)**
    *   **Existing UAPI:** `SYSCALL_VMM_MAP_PAGE` with `bharat_sys_vmm_map_page_args_t`.
    *   **Actual implemented kernel handler:** `bh_sys_vmm_map_page()`, utilizing `vmm_map_page()`.
    *   **Target VM-space identity:** It targets the `kernel_space` implicit global mapping, not an explicit target VM-space identity.
    *   **Required capability:** `CAP_TYPE_MEMORY` with `BH_CAP_RIGHT_MEMORY_MAP`.
    *   **Supported memory profiles:** Unknown context outside kernel space.
    *   **Missing behavior:** Maps into the global kernel space. It cannot inject mappings into a user-specified isolated VM space via capability (as required by the userspace `vm_manager`).
    *   **Status:** BLOCKED.

4.  **Unmap Region (`unmap`)**
    *   **Existing UAPI:** `SYSCALL_VMM_UNMAP_PAGE` with `bharat_sys_vmm_unmap_page_args_t`.
    *   **Actual implemented kernel handler:** `bh_sys_vmm_unmap_page()`, utilizing `vmm_unmap_page()`.
    *   **Target VM-space identity:** Targets the `kernel_space` implicitly.
    *   **Required capability:** `CAP_TYPE_MEMORY` with `BH_CAP_RIGHT_MEMORY_UNMAP`.
    *   **Supported memory profiles:** Unknown context outside kernel space.
    *   **Missing behavior:** Unmaps from the global kernel space, lacking ability to act on a specified VM space.
    *   **Status:** BLOCKED.

5.  **Protect Region (`protect`)**
    *   **Existing UAPI:** None.
    *   **Actual implemented kernel handler:** `prot_domain_protect` or similar internal kernel functions.
    *   **Target VM-space identity:** Missing.
    *   **Required capability:** `CAP_TYPE_VM_SPACE` with `BH_CAP_RIGHT_WRITE` / `PAGE_PROTECT`.
    *   **Supported memory profiles:** Architecture dependent (MMU/MPU capabilities).
    *   **Missing behavior:** No capability-secured UAPI to adjust protections on an existing mapping in a specified VM space (and validate W^X rules at the kernel boundary).
    *   **Status:** BLOCKED.

6.  **Query Region (`query`)**
    *   **Existing UAPI:** None.
    *   **Actual implemented kernel handler:** Internal tree walk over `vm_region_t` linked lists or VMA trees.
    *   **Target VM-space identity:** Missing.
    *   **Required capability:** `CAP_TYPE_VM_SPACE` with `BH_CAP_RIGHT_READ`.
    *   **Supported memory profiles:** All.
    *   **Missing behavior:** No capability-secured UAPI to query the exact status, size, and protection of memory mappings in a given VM space.
    *   **Status:** BLOCKED.

---

## B. Root Cause

The core issue stems from the fact that `SYSCALL_VMM_MAP_PAGE` and `SYSCALL_VMM_UNMAP_PAGE` were historically designed to operate on the global `kernel_space` context. They accept an argument for the capability of the physical memory (`cap_id` targeting `CAP_TYPE_MEMORY`), but they **do not accept a capability identifying the target VM space**.

Because page mapping into `kernel_space` does not implement capability-secured mappings into an explicitly identified user VM space, the userspace `vm_manager` service has no way to configure memory for newly created processes or enforce per-space protection rules. The `vm_manager` cannot reliably allocate, map, modify, or query isolated address spaces.

---

## C. Proposed Kernel Contract

Instead of allocating new syscall numbers, we propose leveraging the existing `SYSCALL_CAPABILITY_INVOKE` (already used for `BH_PROCESS_OP_*` and `BH_BOOTSTRAP_OP_*`). We should define a new capability object type for VM Spaces (`CAP_TYPE_VM_SPACE`) and introduce a set of operations (`BH_VMM_OP_*`).

The canonical kernel path will be:
`VM service -> SYSCALL_CAPABILITY_INVOKE -> kernel cap validation -> VMM space operation -> aspace/region/vm_object -> HAL page-table backend`

Proposed capabilities and semantics:

1.  **`vm_space_create(profile, timing, ...)`**
    *   **Opcode:** (e.g. via `BH_BOOTSTRAP_OP_CREATE_VM_SPACE` or a dedicated resource manager endpoint).
    *   **Semantics:** Allocates a new `vm_space_t` within the kernel for the requested `mem_profile_t` and `vm_timing_class_t`. Yields a new `CAP_TYPE_VM_SPACE` capability handle.

2.  **`vm_space_destroy(space_cap)`**
    *   **Opcode:** `BH_VMM_OP_DESTROY`
    *   **Semantics:** Requires `BH_CAP_RIGHT_DESTROY`. Frees the `vm_space_t`, invoking `aspace_destroy`, tearing down all mappings and page tables. Revokes all handles referencing this space.

3.  **`vm_map(space_cap, memory_cap, vaddr, length, protection, ...)`**
    *   **Opcode:** `BH_VMM_OP_MAP`
    *   **Semantics:** Requires `BH_CAP_RIGHT_MEMORY_MAP` on `space_cap`, and `BH_CAP_RIGHT_READ`/`WRITE` on `memory_cap`. Authenticates bounds and alignment. Evaluates W^X restrictions. Injects mapping via `prot_domain_map_region` into the isolated `vm_space_t` targeted by `space_cap`. Handles rollback on partial mapping failures.

4.  **`vm_unmap(space_cap, vaddr, length)`**
    *   **Opcode:** `BH_VMM_OP_UNMAP`
    *   **Semantics:** Requires `BH_CAP_RIGHT_MEMORY_UNMAP` on `space_cap`. Safely unmaps the specified range from the target VM space and forces a TLB invalidation (`tlb_invalidate_all` on the space).

5.  **`vm_protect(space_cap, vaddr, length, protection)`**
    *   **Opcode:** `BH_VMM_OP_PROTECT`
    *   **Semantics:** Requires `BH_CAP_RIGHT_MEMORY_PROTECT` / `WRITE`. Alters permissions of the existing mapping. Enforces strict W^X checking before applying architectural changes. TLB flushing must be strictly coordinated across owning cores.

6.  **`vm_query(space_cap, vaddr, ...)`**
    *   **Opcode:** `BH_VMM_OP_QUERY`
    *   **Semantics:** Requires `BH_CAP_RIGHT_READ`. Returns metadata about the region encompassing `vaddr` (size, flags, mapping type) using the kernel's canonical `vm_region_t` lists.

All operations must include strict owner-core logic (from `vm_space.c`) to ensure TLB completions match the firm/hard real-time scheduling constraints.

---

## D. Kernel Implementation Follow-up

**Proposed Task:** `BH-VM-KERNEL-P0-002 — Expose Capability-Secured VM-Space Operations`

**Details:**
*   Extend `SYSCALL_CAPABILITY_INVOKE` in `core/kernel/src/personality/native/native_syscall_handlers.c` (or via `cap_invoke()` in `cap_dispatch.c`) to handle `CAP_TYPE_VM_SPACE` operations.
*   Implement standard opcodes in `interface/include/bharat/uapi/system/vmm.h` (or similar).
*   Implement `cap_lookup_vm_space()` and attach it to the rights framework.
*   Ensure that operations map perfectly over `mm_vmm_map_page`, `aspace_create`, and `aspace_destroy` from the core memory management subsystem.
*   **Acceptance Tests:** New kernel tests verifying `SYSCALL_CAPABILITY_INVOKE` can accurately create, map, and tear down an isolated VM space via valid capability delegation.

Once `BH-VM-KERNEL-P0-002` is completed, the blocked task `BH-VM-P0-001` can be unblocked, and `kernel_adapter.c` can be safely implemented using the newly exposed operations.