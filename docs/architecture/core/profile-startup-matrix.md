---
title: Profile Startup Matrix
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - core
see_also:
  - README.md
---
# Profile Startup Matrix

## Overview

This document defines the normative matrix of how Bharat-OS behaves at runtime across different profiles. It dictates what services must start, what services are optional, and what services are strictly forbidden for a given build profile.

It also defines the system-wide policies that the System Policy Manager (`sysmgr`) must enforce regarding restarts, fault handling, memory, telemetry, and power.

## Profile Matrix

| Profile | Boot Intent | Mandatory Services | Optional Services | Forbidden Services | Restart Policy | Fault Handling Mode | Scheduler Mode | Memory Policy Class | Telemetry Level | Update Policy | Power/Thermal Policy |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Tiny Embedded** | Minimal footprint, single control loop | `sysmgr`, `gpio_mgr` | `uart_shell` | `ui_mgr`, `media_mgr`, `netstack` | Fast Reboot | Reboot System | GP / Round Robin | Flat / No-MMU | Low / Serial Only | JTAG/Serial | Static |
| **Appliance** | Network-centric, bounded compute | `sysmgr`, `netstack`, `netmgr` | `update_mgr`, `telemetry_mgr` | `ui_mgr`, `media_mgr` | Service Restart | Isolate & Restart | GP | MMU / Protected | Medium | OTA / A-B | Moderate Duty Cycling |
| **Desktop / GP** | General purpose compute, rich UI | `sysmgr`, `ui_mgr`, `storage_mgr`, `netstack` | `ai_gov`, `media_mgr` | None | Service Restart | Isolate & Restart | GP / Fair | MMU / Full VM | User Opt-In | Package Manager | Dynamic Frequency Scaling |
| **Edge** | Secure gateway, rich connectivity | `sysmgr`, `netstack`, `update_mgr`, `crypto_mgr` | `ai_gov`, `storage_mgr` | `ui_mgr` | Service Restart | Isolate & Degrade | MIX (GP+RT) | MMU / Protected | High | Secure OTA | Aggressive Duty Cycling |
| **Cloud / DC** | High-throughput, isolated workloads | `sysmgr`, `netstack`, `storage_mgr`, `telemetry_mgr` | `ai_gov` | `ui_mgr`, `media_mgr` | Service Restart | Restart & Dump | Scalable | MMU / NUMA Aware | Very High | Orchestrated | Performance-Biased |
| **Real-Time (RT)** | Deterministic latency | `sysmgr`, `rt_sched_mgr` | `netstack` (Lite) | `ui_mgr`, `media_mgr`, `ai_gov` | Degrade | Isolate & Degrade | RT / EDF / FIFO | Static Allocation | Low / Tracing | Offline / JTAG | Fixed Frequency / No Sleep |
| **Safety** | High reliability, certifiable | `sysmgr`, `watchdog`, `health_mgr` | `storage_mgr` (Log only) | `ui_mgr`, `media_mgr`, `netstack` (Full) | System Reboot | Safe Mode / Reboot | RT / Strict Priority | Static / Partitioned | Low / Audit Log | Certified Offline | Fixed / Monitored |
| **Automotive** | Mixed criticality, safe-state support | `sysmgr`, `can_mgr`, `watchdog` | `netstack`, `media_mgr` (Infotainment) | None | Mixed (Profile dependent) | Safe State Transition | MIX (RT + GP) | MMU / Hypervisor | Medium / OBD | Secure OTA | Monitored |
| **Mobile** | Power sensitive, rich connectivity | `sysmgr`, `ui_mgr`, `netstack`, `power_mgr`, `modem_mgr` | `ai_gov`, `media_mgr` | None | Service Restart | Isolate & Restart | GP / Energy Aware | MMU / Aggressive CoW | Medium | OTA | Aggressive / AI Governor |

## Developer Constraints

1.  **Sysmgr Authority:** The System Policy Manager must read this profile (or its compiled equivalent) and strictly enforce the `Forbidden Services` list. If a forbidden service attempts to register or start, `sysmgr` must deny the capability and log a critical security violation.
2.  **Mandatory Checks:** If a `Mandatory Service` fails to start during the initial boot phase, `sysmgr` must consider the boot a failure and trigger the `Fault Handling Mode` defined for that profile (e.g., Safe Mode or Reboot).
3.  **Memory Allocation:** The memory policy class dictates how the kernel provisions pools. For instance, the RT profile mandates static allocation, meaning the system policy manager must reject any dynamic sizing requests from services.
