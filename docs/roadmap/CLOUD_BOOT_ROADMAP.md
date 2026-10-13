# Bharat-OS Production & Cloud Readiness Review (AWS · Azure · GCP)

**Date**: 2026-10-13
**Reviewed branch**: `developer` @ `4f94c44` ("Developer (#1032)")
**Workstream IDs**: PROD-001 … PROD-030
**Audience**: architect, reviewers, coding agents (same conventions as `BOOT-UX-ANIMATED-SPLASH-PLAN.md`)

## 0. Verdict
Bharat-OS has a strong *architecture* (capability microkernel, ABI-locked syscalls, layer linters, 5-arch QEMU matrix) but it is **not yet deliverable as a cloud VM image** on any of the three clouds. The `delivery/targets/cloud/{aws,azure,gcp}` targets are **QEMU targets with a different profile name**: same `qemu-virt` board, multiboot2 ELF, no cloud drivers, and the profile only changes a string (`DEVICE_PROFILE_NAME "Cloud VM"`).

To boot on a real cloud VM the OS needs, at minimum: a **disk-bootable image** (UEFI/BIOS + bootloader), an **NVMe** driver (shared by all three clouds), **one NIC driver per cloud** (virtio-net/gVNIC, ENA, netvsc/MANA), **correct timekeeping on hypervisors**, a **working TCP/HTTP client** for the metadata service, and an **image packaging pipeline**. Before any of that, the **build and CI must be green and hermetic** — today they are not.

Recommended path: fix build integrity → GCP first (fewest new drivers, virtio-net already exists) → AWS (NVMe + ENA) → Azure (VMBus family, hardest). Position the first cloud release as a **headless, single-purpose "cloud appliance" image**, not a general-purpose server OS.

---

## 1. Boot Composition Architecture
The bootloader should have **six responsibilities only**:
| Boot responsibility | What Bharat-OS needs |
|---|---|
| Protocol | UEFI, Multiboot2, DTB/firmware adapters |
| Verification | Verify manifest/kernel/init image signatures |
| Loading | Kernel + required boot modules + initial root image |
| Discovery | Memory map, ACPI/DTB, framebuffer, CPU boot info |
| Recovery | A/B slot, recovery/safe-mode boot |
| Handoff | Convert everything into one architecture-neutral `bh_boot_context` |

The key addition is a canonical structure `bh_boot_context` where every protocol ends.

## 2. Bharat Boot Manifest
Have the build system generate a boot manifest (YAML) rather than encoding composition in CMake conditionals.

## 3. Extend Target YAML
Target YAML should describe composition, e.g., features, components (drivers, services, stacks, personalities), and security options.

## 4. Component Descriptors
Every major Bharat component should declare its own metadata (`component.yaml`) and Nirmaan resolves the graph automatically.

## 5. Nirmaan Build Pipeline
Nirmaan should be the sole composition authority, generating CMake configs, boot manifests, and root manifests.

## 6. Bootloader choice
Do not force one bootloader across every device.
Support Multiboot2, UEFI (Limine/GRUB2), U-Boot/OpenSBI, and eventually native EFI stub.

## 7. What not to put in the bootloader
Service dependency resolution, cloud clients, scheduling policies, device/power policies, GUI policies, etc. The bootloader just loads and verifies.

## 8. Service Composition
Build-time vs Boot/Runtime composition: The component registry helps at runtime to filter what actually starts based on detected hardware and profiles.

## 9. Generated Component Registry
Generate a component table (`__bh_components[]`) instead of maintaining it manually.

## 10. Composition Validation
Validate dependencies, capabilities, and maturities before CMake compilation.

## 11. Artifact Composition Identity
Embed build, target, architecture, profile, and hash IDs into the final image.

## 12. A/B and Recovery
Add A/B and recovery slots in boot metadata for OTA capabilities.

## 13. Build System Change
CMake should no longer decide product composition. Its job is to build exactly the component graph Nirmaan resolved.

---

# Recommended implementation order
1. **BOOT-COMP-001 — canonical `bh_boot_context`** and adapters for current Multiboot2 + UEFI.
2. **BOOT-COMP-002 — boot manifest v1**, including profile, modules, hashes, slots and security state.
3. **BUILD-COMP-001 — component descriptor schema** for kernel, driver, service, stack and personality components.
4. **BUILD-COMP-002 — Nirmaan dependency/profile/capability resolver** producing `composition.json`.
5. **BUILD-COMP-003 — generated CMake configuration + generated component registry**, eliminating product-selection logic from CMake.
6. **IMG-COMP-001 — generic image builder** for GPT+ESP+kernel+root, then output adapters for RAW/GCP, AWS and fixed VHD/Azure.
7. **SEC-BOOT-001 — manifest/kernel signing and composition hash**.
8. **BOOT-REC-001 — A/B/recovery metadata**, even if OTA implementation comes later.
9. **CI-COMP-001 — composition validation gate** rejecting unsupported dependencies, forbidden stubs and invalid maturity levels.
10. **CI-BOOT-001 — boot-matrix test**: BIOS/UEFI × x86_64/ARM64/RV64 as applicable, eventually cloud-native targets.
