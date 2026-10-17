/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

#include "core/cmdline.h"

static bool is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

/* True if the token [tok, tok+len) has the key `key` (up to '=' or end). */
static bool token_has_key(const char *tok, size_t len, const char *key, size_t *key_len)
{
    size_t i = 0;

    while (key[i] != '\0') {
        if (i >= len || tok[i] != key[i]) {
            return false;
        }
        i++;
    }
    if (i < len && tok[i] != '=') {
        return false; /* "smy.ktestx" does not match "smy.ktest" */
    }
    *key_len = i;
    return true;
}

bool cmdline_find(const char *cmdline, const char *key, const char **value, size_t *value_len)
{
    bool found = false;
    const char *p = cmdline;

    if (cmdline == NULL || key == NULL || key[0] == '\0') {
        return false;
    }
    while (*p != '\0') {
        const char *tok;
        size_t len = 0;
        size_t key_len;

        while (is_space(*p)) {
            p++;
        }
        tok = p;
        while (*p != '\0' && !is_space(*p)) {
            p++;
            len++;
        }
        if (len == 0 || !token_has_key(tok, len, key, &key_len)) {
            continue;
        }
        found = true;
        if (value != NULL) {
            *value = key_len < len ? tok + key_len + 1 : tok + len;
        }
        if (value_len != NULL) {
            *value_len = key_len < len ? len - key_len - 1 : 0;
        }
    }
    return found;
}

bool cmdline_value_is(const char *value, size_t value_len, const char *text)
{
    size_t i = 0;

    if (value == NULL || text == NULL) {
        return false;
    }
    for (; i < value_len; i++) {
        if (text[i] == '\0' || text[i] != value[i]) {
            return false;
        }
    }
    return text[i] == '\0';
}
