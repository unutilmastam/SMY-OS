# Security Policy

## Current state

SMY-OS is in early development (Phase 0). There is no released software and
no security support yet. Do **not** use SMY-OS for real data or on a machine
whose disks you care about.

## Reporting a vulnerability

Until a dedicated address exists, report privately via a **GitHub private
security advisory** on this repository ("Security" tab → "Report a
vulnerability"). Do not open a public issue for security bugs. Please include
affected commit, reproduction steps (QEMU command line if possible) and impact.

Target response: acknowledgement within 7 days. There is no bug bounty.

## Security principles

The detailed model is in `THREAT_MODEL.md`. In short:

- User/kernel separation; every syscall argument validated
  (`copy_from_user` / `copy_to_user` with bounds checks).
- Kernel hardening from the first kernel phases: W^X kernel mappings, NX,
  SMEP/SMAP when available, stack protector, guard pages, no kernel pointers
  leaked to user mode.
- Least privilege: apps receive capabilities, not ambient authority.
- Established cryptographic libraries only; no home-made algorithms.
- Signed packages and releases (Phases 26, 35); private keys never in git.
- Secure Boot: **not supported** until Phase 37 passes.

## Supported versions

None yet. This section will list supported releases once releases exist
(Phase 35).
