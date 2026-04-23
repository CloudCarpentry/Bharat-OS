# Shell Profile Matrix

This matrix maps Bharat-OS product profiles to shell class, access mode, and operational policy.

## Profile-to-shell mapping

| Profile | Default shell class | Visibility | Primary modes | Notes |
|---|---|---|---|---|
| IoT / MPU / MMU-lite | Mini shell or none | Usually hidden | recovery, factory | Favor tiny footprint. Disable shell entirely when serial console is unavailable. |
| Appliance / TV / kiosk | Hidden service mini/admin subset | Hidden | prod, recovery | No end-user interactive shell; service access only. |
| Edge / gateway / network appliance | Admin shell | Operator-facing | prod, dev, recovery | Strong capability gates and audit required. |
| Automotive / drone / robotics | Restricted admin + mini recovery | Hidden/operator | prod, recovery, factory | Safety runtime restricts mutating commands; break-glass path must be explicit. |
| Cloud / server / developer platforms | Full admin shell | Visible | dev, prod, recovery | Broader diagnostics and automation-friendly output. |
| Linux personality target (deferred) | Compatibility shell | Personality-facing | dev/prod (personality-scoped) | Implement later in `personalities/compat/`; keep separate from core system shell. |

## Policy expectations by profile

### IoT / constrained targets

- Prefer `CONFIG_SHELL_MINI` only.
- Cap command count and memory footprint aggressively.
- Remote shell (`CONFIG_SHELL_REMOTE`) disabled by default.

### Appliance / kiosk targets

- Shell not exposed in UX surface.
- Only service channel access with audited authentication.
- Mutating commands disabled unless maintenance mode is active.

### Edge and network appliance targets

- Enable `CONFIG_SHELL_ADMIN`.
- Enforce capability requirements on all non-read commands.
- Structured output mode recommended for automation.

### Safety-critical (automotive/robotics/drone)

- Restricted admin command profile only.
- Recovery shell isolated from normal runtime path.
- Build-time disallow list for commands that can affect safety behavior at runtime.

### Cloud / server / dev targets

- Full admin shell with observability-oriented namespaces.
- Developer mode can include expanded diagnostics, but privilege boundaries still apply.

## Build flag matrix

| Build flag | Tiny/IoT | Appliance | Edge | Safety-critical | Cloud/dev |
|---|---:|---:|---:|---:|---:|
| `CONFIG_SHELL_MINI` | ✓ | optional | optional | ✓ | optional |
| `CONFIG_SHELL_ADMIN` | optional | optional (hidden) | ✓ | restricted | ✓ |
| `CONFIG_SHELL_REMOTE` | usually off | off by default | optional | restricted/off | optional |
| `CONFIG_SHELL_RECOVERY` | optional | ✓ | ✓ | ✓ | ✓ |
| `CONFIG_SHELL_SCRIPT` | off | off | optional | off/restricted | optional |
| `CONFIG_SHELL_COMPAT_POSIX` | n/a | n/a | n/a | n/a | deferred |

## Command policy baseline

| Command class | Dev | Prod | Factory | Recovery | Safety-critical runtime |
|---|---|---|---|---|---|
| Core read-only (`help`, `version`, `status`, `uptime`) | allow | allow | allow | allow | allow |
| Inventory/status (`sys info`, `svc list`, `svc status`, `dev list`, `mem stat`) | allow | read-only | allow | allow | allow (profile-filtered) |
| Diagnostics (`log tail`, `health summary`, bounded diag) | allow | allow (rate-limited where needed) | allow | allow | allow with policy filters |
| Mutating admin ops (`svc restart`, `update apply`, `reboot`) | allow with caps | restricted/cap-gated | allow with caps | limited allow | default deny unless explicitly approved |
| Provisioning/calibration/security enrollment | optional | deny | allow | deny | deny at runtime |

## Review checklist for new profile additions

When adding a new profile, document:

1. Shell class and visibility.
2. Enabled build flags.
3. Allowed modes (`dev/prod/factory/recovery`).
4. Mutating command policy and required capabilities.
5. Audit and rate-limit requirements.
6. Safety restrictions (if applicable).
