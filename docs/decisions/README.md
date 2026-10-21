# Architecture Decision Records

Each significant decision is one file `NNNN-title.md` following
[the template](0000-template.md) (Context, Decision, Alternatives,
Consequences). Accepted ADRs are changed only by a new ADR that supersedes them.

| ADR | Title | Status |
|---|---|---|
| [0001](0001-kernel-architecture.md) | Kernel architecture: modular monolith | Accepted |
| [0002](0002-languages.md) | Languages: C17 freestanding, GNU assembler (Intel syntax), Rust deferred | Accepted |
| [0003](0003-toolchain.md) | Toolchain: Clang + ld.lld | Accepted |
| [0004](0004-build-system.md) | Build system: GNU Make | Accepted |
| [0005](0005-bootloader-limine.md) | Bootloader: Limine v12.9.3 | Accepted |
| [0006](0006-project-licence.md) | Project licence: Apache-2.0 | Accepted |
| [0007](0007-limine-from-source.md) | Limine: build from the verified release source tarball | Accepted (supersedes part of 0005) |
| [0008](0008-boot-architecture.md) | Boot architecture: hybrid ISO, UEFI primary, BIOS secondary | Accepted |
| [0009](0009-ktest-and-smbios-config.md) | In-kernel self-tests, selected through an SMBIOS-supplied boot config | Accepted |
| [0010](0010-physical-memory-bitmap.md) | Physical memory manager: bitmap allocator | Accepted |
| [0011](0011-logo-concept-module.md) | Primary logo direction: concept A "Module" (owner decision) | Accepted |
| [0012](0012-virtual-memory-and-heap.md) | Virtual memory: own page tables, guarded stacks, slab heap | Accepted |
