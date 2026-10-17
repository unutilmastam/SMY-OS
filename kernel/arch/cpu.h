/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/*
 * Architecture-independent CPU interface. Generic kernel code (core/, and
 * drivers for hardware that is not tied to one architecture) includes only
 * this header; each architecture implements it under arch/<name>/
 * (docs/architecture/hardware.md, "Portability rule").
 */

#ifndef SMY_KERNEL_ARCH_CPU_H
#define SMY_KERNEL_ARCH_CPU_H

#include "core/compiler.h"

/* Disable interrupts and stop this CPU permanently. */
SMY_NORETURN void arch_halt_forever(void);

#endif /* SMY_KERNEL_ARCH_CPU_H */
