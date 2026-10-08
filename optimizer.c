#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "toolkit.h"

typedef struct { char name[128]; long long val; } Const;

static int is_word(char c) { return isalnum((unsigned char)c) || c == '_'; }
static int is_op(char c)   { return c == '+' || c == '-' || c == '*' || c == '/'; }
static const char *skip_ws(const char *p) { while (*p && isspace((unsigned char)*p)) p++; return p; }

static const char *take_word(const char *p, char *out, size_t sz) {
    const char *s = p;
    while (is_word(*p)) p++;
    if (p == s) return NULL;
    snprintf(out, sz, "%.*s", (int)(p - s), s);
    return p;
}

static const char *take_int(const char *p, char *out, size_t sz) {
    const char *s = p;
    if (*p == '-') p++;
    const char *d = p;
    while (isdigit((unsigned char)*p)) p++;
    if (p == d) return NULL;
    snprintf(out, sz, "%.*s", (int)(p - s), s);
    return p;
}

/* mode 0: v = int op int      mode 1: v = word op word */
static int match_binary(const char *line, int mode, char *v, char *a, char *op, char *b) {
    const char *p = take_word(line, v, 128);
    if (!p) return 0;
    p = skip_ws(p);
    if (*p != '=') return 0;
    p = skip_ws(p + 1);
    p = mode == 0 ? take_int(p, a, 128) : take_word(p, a, 128);
    if (!p) return 0;
    p = skip_ws(p);
    if (!is_op(*p)) return 0;
    *op = *p++;
    p = skip_ws(p);
    p = mode == 0 ? take_int(p, b, 128) : take_word(p, b, 128);
    if (!p) return 0;
    return *p == 0;
}

static int match_assign(const char *line, char *v, char *a) {
    const char *p = take_word(line, v, 128);
    if (!p) return 0;
    p = skip_ws(p);
    if (*p != '=') return 0;
    p = skip_ws(p + 1);
    p = take_int(p, a, 128);
    if (!p) return 0;
    return *p == 0;
}

static int find_const(Const *c, int n, const char *name) {
    int r = -1;
    for (int i = 0; i < n; i++) if (!strcmp(c[i].name, name)) r = i;
    return r;
}

static long long floordiv(long long a, long long b) {
    long long q = a / b;
    if ((a % b != 0) && ((a < 0) != (b < 0))) q--;
    return q;
}

char **optimize_code(char **lines, int n, int *out_n) {
    Const *consts = (Const *)malloc(sizeof(Const) * (n + 1)); int nc = 0;
    char **out = (char **)malloc(sizeof(char *) * (n + 1)); int no = 0;

    for (int li = 0; li < n; li++) {
        char line[MAX_LINE + 1];
        snprintf(line, sizeof line, "%s", lines[li]); trim(line);
        char v[128], a[128], b[128], op;
        char buf[4 * MAX_LINE];

        if (match_binary(line, 0, v, a, &op, b)) {
            long long x = strtoll(a, NULL, 10), y = strtoll(b, NULL, 10), z = 0;
            int ok = 1;
            if (op == '+') z = x + y;
            else if (op == '-') z = x - y;
            else if (op == '*') z = x * y;
            else if (op == '/' && y != 0) z = floordiv(x, y);
            else ok = 0;
            if (ok) {
                int ci = find_const(consts, nc, v);
                if (ci < 0) { ci = nc++; snprintf(consts[ci].name, 128, "%s", v); }
                consts[ci].val = z;
                snprintf(buf, sizeof buf, "%s = %lld", v, z);
                out[no++] = xstrdup(buf);
                continue;
            }
        }

        if (match_binary(line, 1, v, a, &op, b)) {
            char sa[128], sb[128];
            int ia = find_const(consts, nc, a), ib = find_const(consts, nc, b);
            if (ia >= 0) snprintf(sa, sizeof sa, "%lld", consts[ia].val); else snprintf(sa, sizeof sa, "%s", a);
            if (ib >= 0) snprintf(sb, sizeof sb, "%lld", consts[ib].val); else snprintf(sb, sizeof sb, "%s", b);
            if (op == '+' && !strcmp(sb, "0")) { snprintf(buf, sizeof buf, "%s = %s", v, sa); out[no++] = xstrdup(buf); continue; }
            if (op == '*' && !strcmp(sb, "1")) { snprintf(buf, sizeof buf, "%s = %s", v, sa); out[no++] = xstrdup(buf); continue; }
            if (op == '*' && !strcmp(sb, "0")) {
                int ci = find_const(consts, nc, v);
                if (ci < 0) { ci = nc++; snprintf(consts[ci].name, 128, "%s", v); }
                consts[ci].val = 0;
                snprintf(buf, sizeof buf, "%s = 0", v); out[no++] = xstrdup(buf); continue;
            }
            snprintf(buf, sizeof buf, "%s = %s %c %s", v, sa, op, sb);
            out[no++] = xstrdup(buf); continue;
        }

        if (match_assign(line, v, a)) {
            int ci = find_const(consts, nc, v);
            if (ci < 0) { ci = nc++; snprintf(consts[ci].name, 128, "%s", v); }
            consts[ci].val = strtoll(a, NULL, 10);
        }
        out[no++] = xstrdup(line);
    }
    free(consts);
    *out_n = no;
    return out;
}
