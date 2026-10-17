/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright 2026 The SMY-OS Authors */

#include "core/fmt.h"

#include <stdbool.h>

size_t fmt_u64(char *buf, size_t size, uint64_t value, unsigned base)
{
    static const char digits[] = "0123456789abcdef";
    char tmp[FMT_U64_BUF_SIZE];
    size_t len = 0;

    if (buf == NULL || size == 0) {
        return 0;
    }
    buf[0] = '\0';
    if (base < 2 || base > 16) {
        return 0;
    }

    /* Digits come out least significant first; reverse them afterwards. */
    do {
        tmp[len++] = digits[value % base];
        value /= base;
    } while (value != 0);

    if (len + 1 > size) {
        return 0;
    }
    for (size_t i = 0; i < len; i++) {
        buf[i] = tmp[len - 1 - i];
    }
    buf[len] = '\0';
    return len;
}

enum length_mod { LEN_DEFAULT, LEN_CHAR, LEN_SHORT, LEN_LONG, LEN_LLONG, LEN_SIZE };

struct spec {
    bool left;
    bool zero;
    unsigned width;
    bool has_precision;
    bool precision_from_arg; /* ".*" */
    unsigned precision;
    enum length_mod length;
};

struct out {
    fmt_putc_fn putc;
    void *ctx;
    size_t count;
};

static void emit(struct out *o, char c)
{
    o->putc(c, o->ctx);
    o->count++;
}

static void emit_padded(struct out *o, const char *text, size_t len, const struct spec *sp,
                        bool negative)
{
    size_t total = len + (negative ? 1u : 0u);
    size_t pad = sp->width > total ? sp->width - total : 0;

    if (!sp->left && !sp->zero) {
        for (size_t i = 0; i < pad; i++) {
            emit(o, ' ');
        }
    }
    if (negative) {
        emit(o, '-');
    }
    if (!sp->left && sp->zero) {
        for (size_t i = 0; i < pad; i++) {
            emit(o, '0');
        }
    }
    for (size_t i = 0; i < len; i++) {
        emit(o, text[i]);
    }
    if (sp->left) {
        for (size_t i = 0; i < pad; i++) {
            emit(o, ' ');
        }
    }
}

/* va_list wrapped in a struct, so helpers can take it by pointer portably
 * (va_list may be an array type) and keep advancing the caller's list. */
struct arg_list {
    va_list ap;
};

/*
 * clang-analyzer's valist checker does not follow va_copy() into a struct
 * that is then passed by pointer, and reports every va_arg() below as reading
 * an uninitialised list (false positive: fmt_vformat initialises it with
 * va_copy before any call). The host unit tests run these paths under
 * ASan/UBSan.
 */
// NOLINTBEGIN(clang-analyzer-valist.Uninitialized)
static uint64_t fetch_unsigned(enum length_mod length, struct arg_list *args)
{
    switch (length) {
    case LEN_CHAR:
        return (unsigned char)va_arg(args->ap, unsigned int);
    case LEN_SHORT:
        return (unsigned short)va_arg(args->ap, unsigned int);
    case LEN_LONG:
        return va_arg(args->ap, unsigned long);
    case LEN_LLONG:
        return va_arg(args->ap, unsigned long long);
    case LEN_SIZE:
        return va_arg(args->ap, size_t);
    case LEN_DEFAULT:
    default:
        return va_arg(args->ap, unsigned int);
    }
}

static int64_t fetch_signed(enum length_mod length, struct arg_list *args)
{
    switch (length) {
    case LEN_CHAR:
        return (signed char)va_arg(args->ap, int);
    case LEN_SHORT:
        return (short)va_arg(args->ap, int);
    case LEN_LONG:
        return va_arg(args->ap, long);
    case LEN_LLONG:
        return va_arg(args->ap, long long);
    case LEN_SIZE:
        /* %zd: the signed type matching size_t. */
        return (int64_t)va_arg(args->ap, size_t);
    case LEN_DEFAULT:
    default:
        return va_arg(args->ap, int);
    }
}

static void emit_number(struct out *o, uint64_t value, unsigned base, bool upper, bool negative,
                        const struct spec *sp)
{
    char digits[FMT_U64_BUF_SIZE];
    size_t len = fmt_u64(digits, sizeof digits, value, base);

    if (upper) {
        for (size_t i = 0; i < len; i++) {
            if (digits[i] >= 'a' && digits[i] <= 'f') {
                digits[i] = (char)(digits[i] - 'a' + 'A');
            }
        }
    }
    emit_padded(o, digits, len, sp, negative);
}

/* Length of `s`, reading at most `max` bytes (so it may lack a NUL). */
static size_t string_length(const char *s, size_t max)
{
    size_t n = 0;

    while (n < max && s[n] != '\0') {
        n++;
    }
    return n;
}

static bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

/* Parse flags, width and length after '%'; returns the conversion char's
 * position. */
static const char *parse_spec(const char *p, struct spec *sp)
{
    *sp = (struct spec){0};
    for (;; p++) {
        if (*p == '-') {
            sp->left = true;
        } else if (*p == '0') {
            sp->zero = true;
        } else {
            break;
        }
    }
    while (is_digit(*p)) {
        if (sp->width < 1000) { /* clamp absurd widths */
            sp->width = sp->width * 10 + (unsigned)(*p - '0');
        }
        p++;
    }
    if (*p == '.') {
        sp->has_precision = true;
        p++;
        if (*p == '*') {
            sp->precision_from_arg = true;
            p++;
        } else {
            while (is_digit(*p)) {
                if (sp->precision < 100000) {
                    sp->precision = sp->precision * 10 + (unsigned)(*p - '0');
                }
                p++;
            }
        }
    }
    if (p[0] == 'h' && p[1] == 'h') {
        sp->length = LEN_CHAR;
        p += 2;
    } else if (p[0] == 'h') {
        sp->length = LEN_SHORT;
        p++;
    } else if (p[0] == 'l' && p[1] == 'l') {
        sp->length = LEN_LLONG;
        p += 2;
    } else if (p[0] == 'l') {
        sp->length = LEN_LONG;
        p++;
    } else if (p[0] == 'z') {
        sp->length = LEN_SIZE;
        p++;
    }
    return p;
}

size_t fmt_vformat(fmt_putc_fn putc, void *ctx, const char *fmt, va_list args)
{
    struct out o = {.putc = putc, .ctx = ctx, .count = 0};
    struct arg_list al;

    va_copy(al.ap, args);
    for (const char *p = fmt; *p != '\0'; p++) {
        struct spec sp;
        const char *start = p;

        if (*p != '%') {
            emit(&o, *p);
            continue;
        }
        p = parse_spec(p + 1, &sp);
        if (sp.precision_from_arg) {
            const int prec = va_arg(al.ap, int);
            /* A negative precision argument means "no precision". */
            sp.has_precision = prec >= 0;
            sp.precision = prec >= 0 ? (unsigned)prec : 0;
        }
        switch (*p) {
        case '%':
            emit(&o, '%');
            break;
        case 'c': {
            const char c = (char)va_arg(al.ap, int);
            emit_padded(&o, &c, 1, &sp, false);
            break;
        }
        case 's': {
            const char *s = va_arg(al.ap, const char *);
            if (s == NULL) {
                s = "(null)";
            }
            sp.zero = false;
            emit_padded(&o, s, string_length(s, sp.has_precision ? sp.precision : SIZE_MAX), &sp,
                        false);
            break;
        }
        case 'd':
        case 'i': {
            const int64_t v = fetch_signed(sp.length, &al);
            /* Negate in unsigned arithmetic: well defined for INT64_MIN. */
            const uint64_t magnitude = v < 0 ? 0u - (uint64_t)v : (uint64_t)v;
            emit_number(&o, magnitude, 10, false, v < 0, &sp);
            break;
        }
        case 'u':
            emit_number(&o, fetch_unsigned(sp.length, &al), 10, false, false, &sp);
            break;
        case 'x':
        case 'X':
            emit_number(&o, fetch_unsigned(sp.length, &al), 16, *p == 'X', false, &sp);
            break;
        case 'p': {
            const uintptr_t v = (uintptr_t)va_arg(al.ap, void *);
            const struct spec ptr_spec = {.zero = true, .width = 16};
            emit(&o, '0');
            emit(&o, 'x');
            emit_number(&o, v, 16, false, false, &ptr_spec);
            break;
        }
        default:
            /* Unsupported or truncated conversion: print it as written. */
            for (const char *q = start; q <= p && *q != '\0'; q++) {
                emit(&o, *q);
            }
            if (*p == '\0') {
                p--; /* let the loop see the terminator */
            }
            break;
        }
    }
    va_end(al.ap);
    return o.count;
}
// NOLINTEND(clang-analyzer-valist.Uninitialized)

struct buffer_sink {
    char *buf;
    size_t size;
    size_t used;
};

static void buffer_putc(char c, void *ctx)
{
    struct buffer_sink *sink = ctx;

    if (sink->used + 1 < sink->size) {
        sink->buf[sink->used++] = c;
    }
}

size_t fmt_vsnprintf(char *buf, size_t size, const char *fmt, va_list args)
{
    struct buffer_sink sink = {.buf = buf, .size = buf != NULL ? size : 0, .used = 0};
    const size_t total = fmt_vformat(buffer_putc, &sink, fmt, args);

    if (buf != NULL && size > 0) {
        buf[sink.used] = '\0';
    }
    return total;
}

size_t fmt_snprintf(char *buf, size_t size, const char *fmt, ...)
{
    va_list args;
    size_t n;

    va_start(args, fmt);
    n = fmt_vsnprintf(buf, size, fmt, args);
    va_end(args);
    return n;
}
