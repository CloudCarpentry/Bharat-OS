### Digital Cockpit / Infotainment

Digital cockpit is different from ADAS: it needs UI, media, Android/Linux compatibility paths, and strong separation from vehicle-control domains.

Potential value:

- **Domain split:** infotainment, cluster, navigation, voice assistant, media, and vehicle-status display can be separated.
- **Display tiers:** headless, text console, framebuffer, lightweight embedded UI, and full compositor can be enabled by profile.
- **Multi-personality direction:** Linux/Android personality work can support app/runtime compatibility without moving those compatibility layers into the kernel.
- **Safety-aware UI isolation:** cluster-critical display paths can be separated from entertainment or third-party app domains.

Example mapping:

| Cockpit function | Bharat-OS mapping |
| --- | --- |
| Instrument cluster | Lightweight UI/framebuffer service |
| Infotainment | Media stack + Android/Linux personality path |
| Navigation | App/service domain |
| Vehicle status | Read-only vehicle data capability |
| Voice/AI assistant | User-space AI/accelerator service |
| Secure warning overlay | Trusted UI/display broker roadmap |
