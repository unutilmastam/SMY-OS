/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

/*
 * Kernel command line parsing (host-testable). The command line is a list
 * of space-separated tokens, each either "key" or "key=value". Values cannot
 * contain spaces. Kernel options use the "smy." prefix
 * (docs/kernel/command-line.md).
 */

#ifndef SMY_KERNEL_CORE_CMDLINE_H
#define SMY_KERNEL_CORE_CMDLINE_H

#include <stdbool.h>
#include <stddef.h>

/*
 * Look up `key`. Returns true if a token "key" or "key=value" exists; then
 * *value / *value_len describe the value (empty for a bare "key"). The last
 * occurrence wins. `value` and `value_len` may be NULL.
 */
bool cmdline_find(const char *cmdline, const char *key, const char **value, size_t *value_len);

/* True if the found value equals `text` exactly. */
bool cmdline_value_is(const char *value, size_t value_len, const char *text);

#endif /* SMY_KERNEL_CORE_CMDLINE_H */
