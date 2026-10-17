# ADR 0006 — Project licence: Apache-2.0

- **Status:** Accepted
- **Date:** 2026-10-17
- **Phase:** 0

## Context

SMY-OS needs a project licence before code is written, so that every
contribution is made under known terms. The comparison of MIT, Apache-2.0,
GPL-3.0-only and MPL-2.0 is in `LICENSES.md` §4. Constraints:

- The project implements compatibility with commercial platforms (Windows PE,
  NT-like services), so an explicit patent grant from contributors matters.
- Third-party components already chosen (Limine, BSD-2-Clause) must be
  compatible.
- The owner wants broad adoption, including by companies.

## Decision

The owner chose **Apache License 2.0** (SPDX: `Apache-2.0`).

- The full licence text is in `LICENSE` at the repository root (unmodified
  official text from <https://www.apache.org/licenses/LICENSE-2.0.txt>).
- `NOTICE` names the project and copyright holder.
- Every source file starts with `SPDX-License-Identifier: Apache-2.0` and the
  copyright line `Copyright 2026 The SMY-OS Authors` (comment syntax of the
  language). Markdown documentation is covered by `LICENSE` without headers.
- Contributions are accepted under Apache-2.0 (inbound = outbound, Apache-2.0
  §5); no separate CLA for now.

## Alternatives

- **MIT** — simplest, but no explicit patent grant.
- **GPL-3.0-only** — prevents closed forks, but lowers company adoption and
  is a harder fit for a future SDK used by third-party applications.
- **MPL-2.0** — file-level copyleft; middle ground, less familiar to most
  contributors.
- **Stay closed (all rights reserved)** — rejected by the owner.

## Consequences

- Others may use, modify and redistribute SMY-OS, including in closed products,
  provided they keep the licence and notices and mark changed files.
- **GPL-2.0-only code can never be included** (Apache-2.0 is incompatible with
  GPLv2). This reinforces the existing rule: no Linux kernel code.
- GPL-3.0/LGPL-3.0 code (e.g. parts of Wine, which is LGPL-2.1-or-later) could
  only be combined under (L)GPL-3.0 terms; any such use needs a new ADR.
- Revisit only if the owner decides to change the licence (requires consent of
  all copyright holders of existing contributions).
