---
title: 'ADR-023: Bharat Shell diagnostic control plane'
status: Draft
owner: Team
last_updated: '2024-01-01'
tags:
- doc
see_also:
- none
---
# ADR-023: Bharat Shell diagnostic control plane

## Status

Accepted

## Decision

The native Bharat Shell is the console-first administration and debugging surface. Shell command handlers may format requests and responses, but they must obtain runtime state only through capability-backed service or kernel diagnostic endpoints. They must never read kernel or service globals directly.

The first contract is `bh_shell_diag_request_v1_t`: a versioned, fixed-width, by-value request containing a bounded argument. Diagnostic services own discovery and mutable state. The shell owns no diagnostic cache. A missing endpoint, unsupported query, malformed request, or insufficient capability fails closed and produces an unavailable/forbidden shell result rather than fabricated data.

Read-only observations may be enabled in production profiles. Allocation and benchmark operations require the diagnostic capability and are disabled in production by default. Providers remain responsible for narrower capability, object, rights, scope, and liveness checks before accessing their subsystem.

## Consequences

This establishes the command vocabulary without waiting for POSIX compatibility and makes service boundaries observable. Runtime bootstrap still needs to bind the backend to real endpoints per target; until then the production backend explicitly returns unsupported. The wire request contains no pointers, handles, architecture-sized integers, or mutable shared state.
