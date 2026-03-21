# Drivers Subcomponents Architecture (Status + Roadmap Mapping)

This document captures driver-layer decomposition with architecture concerns, current status, and next-step closure.

## Mermaid (driver architecture)

```mermaid
%%{init: {'theme':'base','themeVariables':{'primaryColor':'#283618','primaryTextColor':'#ffffff','lineColor':'#dda15e','fontFamily':'Inter'}}}%%
graph TD
    D[Driver Layer] --> BUS[Bus Drivers]
    D --> NET[Network Drivers]
    D --> DISP[Display Drivers]
    D --> ACC[Accelerator Drivers]

    BUS --> PCI[PCI/Platform discovery]
    NET --> VIRTIO[virtio-net path]
    DISP --> FB[Framebuffer path]
    ACC --> GPU[NPU/GPU DMA path]

    classDef partial fill:#bc6c25,stroke:#8b4513,color:#fff;
    classDef scaffold fill:#d00000,stroke:#7f1d1d,color:#fff;

    PCI:::partial
    VIRTIO:::partial
    FB:::partial
    GPU:::scaffold
```

## PlantUML (driver layering)

```plantuml
@startuml
!theme sandstone
skinparam backgroundColor #1f2937
skinparam rectangleBackgroundColor #374151
skinparam rectangleBorderColor #f59e0b
skinparam ArrowColor #93c5fd

rectangle "Driver Layer" {
  rectangle "Bus (PCI/platform)"
  rectangle "Network (virtio/NIC)"
  rectangle "Display (framebuffer/gpu)"
  rectangle "Accelerator (DMA/NPU/GPU)"
}

rectangle "IOMMU Policy Layer"
rectangle "HAL Interrupt + MMIO"

"Driver Layer" --> "IOMMU Policy Layer"
"Driver Layer" --> "HAL Interrupt + MMIO"
@enduml
```

## Driver status matrix

| Driver domain | Arch considerations | Current status | Done | To do | Roadmap linkage |
| --- | --- | --- | --- | --- | --- |
| Bus discovery and platform plumbing | x86 ACPI/PCI, arm64/riscv FDT paths | Partial | Baseline discovery hooks and driver entry points exist. | Improve validation depth and richer platform compatibility tests. | Phase 1 |
| Network drivers (virtio/NIC path) | x86/arm64/riscv virt platforms | Partial | Netstack integration path and adapter scaffolding exist. | Complete fast path, stability tests, and policy enforcement. | Phase 3 |
| Display drivers | Boot framebuffer across profiles | Partial | Boot display + framebuffer baseline path present. | Compositor-era pipeline and acceleration maturation. | Phase 2, Phase 4 |
| Accelerator drivers | DMA, NPU, GPU isolation | Scaffold | Manager and architecture intent documented. | Real map/unmap lifecycle, zero-copy pipelines, IOMMU depth. | Phase 3 |
| Storage drivers | Edge and cloud profile backends | Scaffold/Partial | Service and architecture contracts exist. | NVMe/flash maturity and recovery workflows. | Phase 2, Phase 3 |
