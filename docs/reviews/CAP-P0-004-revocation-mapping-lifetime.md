# CAP-P0-004: capability revocation and mapping lifetime audit

Status: investigation complete; integration defects remain open. Priority: P0.
Base: `developer`, `0de9c1e32217c5b30af26d5ae3f7a9a08d06497c`.
The initial trace and tests used `9465b301`; a final remote fetch found TCP commit #1064, which was fast-forwarded before final validation. It does not change the audited capability, VM or DMA paths. The integrated capability revocation invariant/sorting change `63efdee7` (#1056) is in this base's history.
Scope: new tests and this audit only. No changes to capability revoke, TLB, DMA, VM, HAL or service production code. No production implementation work was assigned.

## Finding and confidence

**A mapping-lifetime integration gap exists at the kernel API boundary.** Successful `cap_table_revoke` invalidates capability entries but does not complete CPU mapping removal or DMA buffer teardown. Production entry points with fake hardware demonstrate that an established mapping remains installed after revoke returns, including after a remote capability ACK. A copied pre-revoke memory authority can also be used to install a mapping afterward. The DMA buffer's stale handle cannot call its unmap API, so revoking the caller's only cleanup authority can strand the mapping.

This is not a claim based on the absence of a TLB call in `cap_revoke.c`. The evidence combines the actual authorization-to-map call paths, the stored mapping identities, the actual revoke/ACK paths, and executable map/revoke/explicit-unmap controls. **End-to-end unauthorized CPU loads or device DMA on hardware have not been reproduced.** The tests do not execute syscalls, real page tables, IOTLBs, devices or concurrent cores. Existing boot failures prevent an end-to-end QEMU access test. Whether `cap_table_revoke` is intended to revoke only future authority must be made explicit; it currently cannot satisfy a promise that all derived accesses are disabled on return.

The independent `bh_dma_grant_revoke` API has a confirmed completion-contract defect: it ignores `iommu_unmap` errors and publishes `REVOKED`/`K_OK` anyway. Its current hardware map path supplies a NULL domain, which the actual `iommu_map` rejects. The fault-injection test deliberately permits that domain to reach ACTIVE and reproduce the completion error. It proves the grant's response to backend failure, **not** a reachable ACTIVE hardware grant or an actual device exploit. The no-IOMMU identity fallback has no isolation and no device-stop operation; software state alone cannot disable bus-master access.

## Verified call graph

These are distinct paths, not an inferred single chain:

| Entry / path | Actual calls and retained state | Completion / limitation |
|---|---|---|
| [Native CPU map](../../core/kernel/src/personality/native/native_syscall_handlers.c#L94) | usercopy/range check -> `bh_syscall_cap_lookup_memory` -> `cap_lookup_memory` -> `cap_validate_ex` -> copied physical base -> `vmm_map_page` -> `mm_vmm_map_page` -> MPA `map_page` -> local flush | Native wrapper targets `kernel_space`, not an explicitly selected user ASpace. It does not record cap locator/epoch in the mapping. User access in a running process is not established by the host test. |
| [Native CPU unmap](../../core/kernel/src/personality/native/native_syscall_handlers.c#L110) | MEMORY/UNMAP lookup -> `vmm_unmap_page` -> `mm_vmm_unmap_page` -> MPA `unmap_page` -> local flush | Stale handle is denied; this is a separate explicit operation. The inspected direct MPA path does not invoke the distributed TLB coordinator. |
| [Local revoke](../../core/kernel/src/cap/cap_revoke.c#L50) | find root/generation under CSpace lock -> REVOKING -> unlink parent under ordered table locks -> increment epoch -> publish transaction -> wait for capability ACKs -> walk locators -> clear rights/object_ref, advance generation, free slots -> return 0 | No retained mapping dependency, object release, CPU-unmap or DMA-unmap completion is joined to this transaction. |
| [Remote revoke](../../core/kernel/src/cap/cap_dispatch.c#L110) | owner registry scan -> invalidate LIVE entries with matching immediate parent fields/epoch -> unlock tables -> send `URPC_CAP_REVOKE_ACK` | ACK certifies that scan, not PTE/IOTLB removal. Immediate remote child tested; deeper remote derivation and failure/race behavior not proven. The match omits parent CSpace ID; exact lineage isolation needs a separate capability-owner review. |
| [DMA buffer map](../../core/kernel/src/mm/dma/dma.c#L322) | current CSpace -> `cap_table_lookup(DMA_GRANT, DMA_MAP)` -> object_ref must equal buffer pointer -> pin/state checks -> optional `hal_iommu_map` -> `hal_dma_map_buffer` -> set mapped/device-owned/direction | Buffer stores domain/IOVA and ownership, but no capability dependency or revoke epoch. Kernel-driver no-CSpace path is distinct from userspace. |
| [DMA buffer unmap](../../core/kernel/src/mm/dma/dma.c#L388) | DMA_GRANT/UNMAP lookup + pointer equality -> sync -> `hal_dma_unmap_buffer` -> optional `hal_iommu_unmap` -> clear ownership on success | No automatic call from capability revoke. IOMMU errors are propagated here; the HAL DMA unmap hook is void. Quiescence and IOTLB completion must be supplied by the backend/owner. |
| [Separate grant object](../../core/kernel/src/mm/dma/dma_grant.c#L91) | grant ID indexes independent array -> REVOKING -> `iommu_unmap(NULL,...)` if available -> ignore result -> REVOKED/K_OK | No CSpace lookup or binding between grant ID and `CAP_TYPE_DMA_GRANT`; no generation in the grant ID. Do not conflate this object with `dma_buffer_t`. |
| [Generic IOMMU unmap](../../core/kernel/src/mm/iommu/iommu_map.c#L41) | reject NULL/zero/unavailable -> scan DMA_DOMAIN/WRITE entries for matching domain -> HAL `ops->unmap` | Direct entry scan does not use generation/state validation or CSpace lock. No explicit call to `iommu_invalidate_range` here; a backend may invalidate internally. |
| [IOMMU invalidation](../../core/kernel/src/mm/iommu/iommu_domain.c#L110) | local HAL invalidate -> publish origin-core request -> uRPC -> receiver HAL invalidate -> ACK count -> wait | Separate API, not called by cap revoke. Shared request contains domain pointer, counter-only ACKs, unbounded wait and ignored receiver status; cannot be reused as a proven bounded revocation barrier. |
| [Distributed VM unmap](../../core/kernel/src/mm/vm/distributed/vm_mapping.c#L127) | mutation reservation + region snapshot -> release VM-space lock -> local protection-domain unmap -> synchronous monitor unmap -> reacquire lock -> detach/generation on success, poison on failure | This is closer to an owner/completion protocol but is not linked to capability revoke. Local unmap result is ignored. Monitor ACK certifies protection-domain unmap result, not an independently verified TLB fence. |
| [Monitor receiver](../../core/services/system/monitord/mon_vm_dispatch.c#L118) | resolve realized space -> `prot_domain_unmap_region` -> update replay/state -> ACK/NACK | MMU backends route to `hal_pt_unmap_range`; the common wrapper delegates leaf clearing without an explicit cross-core TLB join. Exact backend hardware barriers need target-specific validation. |
| [Service VM unmap](../../core/services/vm_manager/vm_manager.c#L520) | service space/region handles -> installed authority `unmap` -> only on success mark/revoke region handle | Positive counterexample: region handle removal follows backend success. Latest service `main` fails closed if authority ops are not installed; default backend cannot provide real mappings. Service handles are not automatically CSpace revocation descendants. |

`cap_invoke` is not a generic user revoke route: the weak trap implementation returns UNSUPPORTED, and the bootstrap implementation handles bootstrap operations. The test invokes kernel APIs directly. No user-facing exploit entry or capability minting authority is assumed.

## Which authority controls existing access?

Use the implemented enum in [capability.h](../../core/kernel/include/capability.h), not the proposed FRAME/VSPACE vocabulary in older documentation.

| Implemented type / identity | Authority checked | Effect on an existing mapping |
|---|---|---|
| `CAP_TYPE_MEMORY` | MAP/UNMAP; lookup returns physical base, fixed 4096-byte size and flags | Checks future API calls. PTE and cached translation have no cap locator. No automatic removal demonstrated. Framebuffer handoff also mints this type. |
| `CAP_TYPE_DMA_GRANT` referencing `dma_buffer_t` | DMA_MAP / MEMORY_UNMAP and exact buffer pointer | Checks map/unmap API entry; active device translation/ownership is independent afterward. |
| `CAP_TYPE_DMA_DOMAIN` | WRITE for generic map/unmap, BIND for attach | Domain remains a separate object; entry revocation does not unmap all IOVAs or detach devices. |
| `CAP_TYPE_ACCEL_BUFFER`, NET_BUFFER and HMEM | Policy exposes mapping/sync/share rights | These names/rights alone do not prove an installed mapping dependency. HMEM has its own generation-bearing registry, CPU map count and HAL device mapping records. No call from CSpace revoke was found. HMEM/system allocation and accelerator paths were inspected, not exercised by this focused harness. |
| Service VM handles / `vm_object_t` | Service lookup plus backend authority; independent refcounts | Separate namespace and lifetime. Object retention does not imply revocation of access; capability deletion does not imply final object reference release. |

[CSpace destroy](../../core/kernel/src/cap/cap_cspace.c#L170) unpublishes its registry and frees CSpace storage; it does not walk/release each underlying object or mapping. Capability entries store an untyped object_ref without retain/release hooks. [VM object release](../../core/kernel/src/mm/vm/objects/vm_object_refcount.c#L21) frees on final reference through ops->release. Device/DMA VM-object release hooks are currently no-ops. Region detach and object refcounts therefore cannot be used as evidence that capability revoke completed teardown.

## Current sequence and completion boundary

```mermaid
sequenceDiagram
    participant Caller
    participant Cap as CSpace owner
    participant Peer as Peer CSpace owner
    participant VM as CPU or DMA mapping owner
    participant HW as Translation backend
    Caller->>Cap: lookup live handle
    Cap-->>Caller: copy of authority (lock released)
    Caller->>VM: map copied object / authorized buffer
    VM->>HW: install translation
    HW-->>VM: map success
    Caller->>Cap: cap_table_revoke(handle)
    Cap->>Cap: REVOKING and new epoch under lock
    Note over Cap: release CSpace locks before waiting
    Cap->>Peer: capability revoke request
    Peer->>Peer: invalidate matching entries
    Peer-->>Cap: capability ACK
    Cap->>Cap: clear local entries and advance generations
    Cap-->>Caller: success
    Note over VM,HW: established mapping unchanged; no teardown request
```

Future authority and established access are different: a failed lookup prevents another map API call, while a CPU load/device DMA normally follows installed translations without calling CSpace validation again. Revocation completion needs an explicit definition for each type. The four safety tests intentionally demand mapping-disable completion and remain expected failures until that contract is integrated.

The lookup-to-map interval is also unprotected. `cap_validate_ex` copies an entry and releases the table lock before a caller uses object_ref. A second validation alone still leaves a race unless mapping publication and revocation are serialized through an epoch/in-flight transaction owned by the resource owner. Merely adding a TLB flush without removing mappings or closing this interval would not fix the observed gap.

## Locks, ownership and ordering

| Component | Observed locking / owner | Required integration constraint |
|---|---|---|
| Capability lookup | One table lock, copy-by-value result, unlock before dispatch | Pin a stable generation-bearing object/dependency or reject on owner epoch; do not retain an unlocked entry pointer. |
| Revoke | Root lock for state/epoch; sorted table locks for parent unlink; per-entry locks for walk; capability wait occurs unlocked | Capture dependency IDs and retain their lifetime under short locks. Release all CSpace locks before VM/device requests or waits. Tests assert both table locks are clear when synchronous uRPC dispatch/ACK occurs. |
| Multiple CSpaces | Revoke sorts `(owner_core,cspace_id,registry_slot)`; local delegation's `cap_lock_two_tables` orders `(numa_node,pointer)` | These orders are not proven compatible. Resolve a single order with capability owner before adding nesting. Potential ABBA is an inspection finding, not a reproduced deadlock. |
| Address-space metadata | `vm_unmap_region` and `aspace_destroy` hold ASpace lock while releasing VM objects and calling protection/HAL paths | No capability-table lock may nest across these calls. Do not add a blocking shootdown inside those locked paths. Backing pages/table storage must remain pinned until CPUs/devices are inaccessible. |
| Distributed VM | VM-space mutation token survives unlock/wait/relock | Preserve serialization and reserve object lifetime while waiting. Compensation in `vm_map` sends unmap after reacquiring space lock: that path also needs memory-owner review before reuse. |
| CPU invalidation | TLB pending request uses active CPU mask, request ID, ACKs, retries/deadline; receiver locally flushes before response | Clear/restrict PTEs first; enforce publication barriers; join bounded target-generation ACKs or prevent further scheduling/access. Errors must be propagated; avoid void wrapper when completion matters. |
| DMA / grant | Buffer state fields and separate global grant array lack a demonstrated per-object revoke serialization | One domain/device owner must close submissions, drain in-flight DMA, unmap and invalidate; prevent ID reuse/new mappings until complete. Capability-table lock is not a device-quiescence mechanism. |
| IOMMU invalidation | Origin-local counter and shared domain pointer, no bounded deadline | Need pointer-free domain identity/generation and status-bearing replay-safe acknowledgements. Counter completion alone is insufficient proof of device IOTLB completion. |

`vm_unmap_region` releases a VM object before protection-domain unmap and ignores the latter's return. `aspace_destroy` sets DYING and frees regions/page-table storage without a visible joined remote-access barrier in that function. These are lifetime-order review targets, not independently reproduced use-after-free exploits in this audit. Do not reuse them as safe revocation primitives without memory-owner signoff.

## Tests and limits

The new [C harness](../../quality/tests/security/cap_revocation_mapping_audit.c) compiles the unchanged production CSpace grant/lookup/delegate/revoke/dispatch, ID allocator, VMM map/unmap, DMA buffer map/unmap, and separate DMA-grant implementation. It stubs HAL translation/scheduling/clock boundaries and the separate grant's generic `iommu_map`/`iommu_unmap` boundary. Generic IOMMU authorization/invalidation code is inspected, not executed in this harness. CSpaces are explicitly initialized in their owner-local registries. Two simulated owners process messages synchronously; no parallelism or hardware execution is claimed.

| Scenario | Observed result |
|---|---|
| Stale local parent/child; naturally reused allocator slot | PASS: revoked handle denied, replacement handle valid, stale revoke cannot remove replacement |
| Immediate remote child | PASS: live before revoke, denied after real handler/ACK processing; dispatch occurs without table locks |
| Established CPU mapping | Gap reproduced: `mapping=1 unmaps=0 flushes_during_revoke=0 acks=1`; explicit unmap positive control removes it |
| Lookup copied before revoke, map afterward | Gap reproduced through actual lookup and VMM calls; demonstrates missing operation serialization, not a threaded race test |
| Active DMA buffer | Gap reproduced: `mapping=1 owned=1 unmaps=0 iommu_unmaps=0 acks=1`; stale map/unmap denied; fresh cleanup authority reaches real unmap and removes backend mapping |
| Separate grant with injected IOMMU timeout | Gap reproduced: revoke K_OK, REVOKED, mapping still present, one unmap call; fake backend accepts NULL domain as explained above |

Run from repository root after tool activation:

```sh
source /workspace/bharat-tools/activate.sh
python3 -m pytest quality/tests/security/test_cap_revocation_mapping_audit.py -ra
python3 -m pytest quality/tests/security/test_cap_revocation_mapping_audit.py --runxfail -q
```

First command: **2 passed, 4 strict expected failures**. It does not certify mapping security. The xfail marker accepts only the dedicated unsafe-observation exception; compiler errors, signals, timeouts and harness/control assertions remain failures. An unexpected pass is a hard failure prompting contract/test review. Second command: **2 passed, 4 failed**, exit 1, exposing the actual unmet security requirements.

Required gates on this base: layer-reference check PASS (existing baseline debt), CMake-dependency check PASS (zero violations), syscall ABI check PASS. Five-target QEMU matrix: all targets build/package, all smoke tests FAIL at pre-existing mandatory early `hw_caps` test (expects frozen state before freeze). This failure is unrelated to the new host tests. Full C host-test preset remains BLOCKED by 44 pre-existing missing-source paths. Real SMP TLB access, IOMMU device DMA, revoke-vs-map concurrency, deep remote lineage, exhaustion/replay and timeout recovery remain untested. No timeouts, assertions or production tests were relaxed.

## Minimum follow-up integration proposal (not implemented)

1. Capability owner defines whether revoke means **future authority only** or **derived access disabled** for each implemented type. Expose distinct completion semantics if both are needed; never label authority-only ACK as memory/device revocation completion.
2. For mapping-bearing types, memory/DMA owners retain bounded dependency records containing object ID/generation, authorizing locator/epoch, ASpace/domain/device ID, VA/IOVA range, permissions and owner. Establish them as part of map publication, not after map returns. Retain backing/object lifetime while a dependency or in-flight operation exists. A delegated capability must identify the lineage being revoked; independently granted authority to the same frame need not be removed automatically.
3. Begin revoke under short CSpace/owner locks: mark relevant lineage REVOKING, advance epoch, deny new operations and snapshot dependency identities/in-flight counts. Release locks. At the resource owner, serialize against map publication: stale-epoch maps are rejected or compensated; already admitted maps must complete into the revocation set before success can be returned.
4. CPU owner removes/restricts translations, releases semantic locks, performs local invalidation and the existing status-returning bounded shootdown for all active realizations, then acknowledges object/epoch/range completion. Inactive CPUs must check generations on re-entry. Do not release frames/table storage before completion. Failure poisons/quarantines and prevents scheduling/access; no success ACK on partial removal.
5. DMA owner blocks submissions and drains/aborts device work, unmaps every dependency, waits for backend-defined IOTLB invalidation and DMA quiescence, propagates unmap errors, then releases pin/IOVA/domain ownership. Kernel-owned cleanup authority must remain usable even after caller capability deletion. Unsupported no-IOMMU isolation cannot be promised by a software REVOKED state.
6. Revoke commits/returns success only after **both** capability-owner and resource-owner acknowledgements for the same transaction/object generation/epoch. Keep failed revocations quarantined and retry boundedly; do not free/reuse grants or backing on failure. Never wait while holding CSpace, ASpace, domain or object spinlocks.

```mermaid
sequenceDiagram
    participant Cap as Capability owner
    participant Mem as Mapping owner
    participant CPU as Active CPU owners
    participant Dev as Device/domain owner
    Cap->>Cap: close epoch and snapshot retained dependencies
    Note over Cap: release CSpace locks
    Cap->>Mem: revoke(object ID, generation, epoch)
    Mem->>Mem: serialize maps; remove PTEs; release locks
    Mem->>CPU: bounded invalidate with generation/request ID
    CPU-->>Mem: flush complete ACKs
    Mem->>Dev: revoke retained DMA dependencies
    Dev->>Dev: stop submissions; drain; unmap; invalidate IOTLB
    Dev-->>Mem: quiescence and invalidation complete, or error
    Mem-->>Cap: access disabled for epoch, or quarantine/error
    Cap->>Cap: commit only after all required completions
```

Coordination prerequisite: the session agent registry contained only the primary investigator. No existing capability/memory/DMA agents were available to contact, and no production-code changes were assigned. Before implementation, obtain joint agreement from those owners on dependency ownership, a uniform lock order, cleanup authority, CPU re-entry and device completion/error contracts. Preserve this investigation's production-file restriction; implement the agreed change in separate follow-up work. Use the new tests as regressions, remove expected-failure annotations only after actual integration closes the corresponding cases, and add real SMP/device tests before claiming complete protection.
