# Bharat-OS Release and Versioning Policy

Bharat-OS uses a manifest-driven release identity system. A single version is not applied monolithically to all services. Instead, the OS uses a component-based versioning approach, combining multiple independently versioned artifacts into a defined release structure.

## Versioning Rules

*   **OS Version**: The top-level product version (e.g., `0.8.0`) represents the combined set of kernel and service components. The OS version changes when the shipped composition of components changes.
*   **Kernel Version**: Advanced when internal kernel changes are made.
*   **Kernel ABI**: Bumped on breaking syscall, capability, or cross-boundary changes. This is the contract for user-land components.
*   **Component Semantic Version**: Internal logic changes within a service or subsystem bump its semver independently.
*   **Component Interface Version**: Bumped when the IPC or schema contract changes for a service.
*   **Release Channel**: Reflects maturity (`stable`, `beta`, `experimental`, `stub`). *Rule:* `stub` and `experimental` components cannot be labeled `stable`.
*   **Release Manifest**: The `os-release.json` manifest is the authoritative bill of materials for any given release.

## Artifacts and Embedding

Every shipped binary includes an embedded `bharat_component_version` descriptor in a dedicated `.bharat.version` ELF section. This ensures tools and the OS can reliably query component version data at runtime, independent of file naming conventions.

For portability and zero relocation overhead during static linking, the struct uses flat POD records without pointer dependencies:

```c
struct bharat_component_version {
    uint32_t magic;
    uint16_t struct_version;
    uint16_t size;
    // ...
    char name[32];
    char kind[16];
    char version[32];
    // ...
};
```

## Signing Rules

Release artifacts are cryptographically signed to maintain provenance and trust:
*   **Hashes**: All artifacts must produce a SHA-256 digest (`.sha256`).
*   **Signatures**: Ed25519 is used for developer signing, producing an associated `.sig` file for the manifest, kernel, and each service.

## CI and Build Pipeline

*   The manifest is compiled into JSON and C headers during the CMake configuration step.
*   The `tools/sign_release.py` script consumes the manifest and build outputs to construct a `dist/Bharat-OS-<version>` release folder.

## Future Plan: GitHub Image Release Flow

To distribute SBC-ready OS images directly from GitHub, Bharat-OS should adopt a tag-driven release pipeline:

1. **Tag triggers release builds**
   - Trigger on pushed tags matching `v*` (for example `v1.0.0`).
2. **Matrix build per board**
   - Run the same build script across a board matrix (for example `raspberrypi4`, `orangepizero`, `generic-arm64`).
3. **Image normalization**
   - Produce consistent file names (`bharat-os-<board>.img`), then compress to `*.img.xz`.
4. **Integrity and provenance**
   - Generate `.sha256` checksums and publish `.sig` detached signatures for each image artifact.
5. **Release publication**
   - Create/update a GitHub Release and attach all compressed images plus checksum/signature files.
6. **Operational hardening**
   - Add optional image-shrink steps before compression and runner disk-space cleanup for large builds.

### Suggested CI shape

- `build-os` job:
  - strategy matrix over boards,
  - installs cross-build dependencies (`qemu-user-static`, `binfmt-support`, `xz-utils`),
  - builds and compresses board image,
  - uploads intermediate artifacts.
- `create-release` job:
  - depends on all matrix builds,
  - downloads artifacts,
  - publishes release assets with release notes.

This flow keeps the release process reproducible while making board-specific images easy for users to discover and download.
