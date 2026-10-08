#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "toolkit.h"

#define MAX_MACROS 64
#define MAX_PARAMS 16

typedef struct {
    char name[128];
    char params[MAX_PARAMS][128]; int nparams;
    char **body; int nbody;
} Macro;

/* Python str.replace(from, to) */
static char *replace_all(const char *s, const char *from, const char *to) {
    size_t fl = strlen(from), tl = strlen(to), sl = strlen(s);
    size_t cap = sl * (tl + 1) + tl + 16;
    char *out = (char *)malloc(cap), *o = out;
    if (fl == 0) {
        memcpy(o, to, tl); o += tl;
        for (size_t i = 0; i < sl; i++) { *o++ = s[i]; memcpy(o, to, tl); o += tl; }
        *o = 0; return out;
    }
    while (*s) {
        if (strncmp(s, from, fl) == 0) { memcpy(o, to, tl); o += tl; s += fl; }
        else *o++ = *s++;
    }
    *o = 0;
    return out;
}

/* line.split(None, 1) on an already-trimmed line */
static void split_first(const char *line, char *first, size_t fsz, const char **rest) {
    size_t i = 0;
    while (line[i] && !isspace((unsigned char)line[i])) i++;
    snprintf(first, fsz, "%.*s", (int)i, line);
    while (line[i] && isspace((unsigned char)line[i])) i++;
    *rest = line + i;
}

/* s.split(",") with each piece stripped */
static int split_commas(const char *s, char out[][128], int max) {
    int n = 0;
    const char *p = s;
    for (;;) {
        const char *q = strchr(p, ',');
        size_t len = q ? (size_t)(q - p) : strlen(p);
        if (n < max) { snprintf(out[n], 128, "%.*s", (int)len, p); trim(out[n]); n++; }
        if (!q) break;
        p = q + 1;
    }
    return n;
}

char *macro_process(char **lines, int n) {
    Macro mnt[MAX_MACROS]; int nm = 0;
    char **exp = (char **)malloc(sizeof(char *) * (n * 64 + 16)); int ne = 0;
    int i = 0;

    while (i < n) {
        char line[MAX_LINE + 1];
        snprintf(line, sizeof line, "%s", lines[i]); trim(line);
        char up[MAX_LINE + 1]; snprintf(up, sizeof up, "%s", line); upper_inplace(up);

        if (!strcmp(up, "MACRO")) {
            if (i + 1 >= n) break;                 /* malformed: no header line */
            char hdr[MAX_LINE + 1]; snprintf(hdr, sizeof hdr, "%s", lines[i + 1]); trim(hdr);
            char name[128]; const char *rest;
            split_first(hdr, name, sizeof name, &rest);
            upper_inplace(name);
            Macro m; memset(&m, 0, sizeof m);
            snprintf(m.name, sizeof m.name, "%s", name);
            if (*rest) m.nparams = split_commas(rest, m.params, MAX_PARAMS);
            m.body = (char **)malloc(sizeof(char *) * (n + 1)); m.nbody = 0;
            i += 2;
            while (i < n) {
                char t[MAX_LINE + 1]; snprintf(t, sizeof t, "%s", lines[i]); trim(t); upper_inplace(t);
                if (!strcmp(t, "MEND")) break;
                m.body[m.nbody++] = xstrdup(lines[i]); i++;
            }
            int idx = -1;
            for (int k = 0; k < nm; k++) if (!strcmp(mnt[k].name, m.name)) idx = k;
            if (idx >= 0) { free_lines(mnt[idx].body, mnt[idx].nbody); free(mnt[idx].body); mnt[idx] = m; }
            else if (nm < MAX_MACROS) mnt[nm++] = m;
        } else {
            char name[128]; const char *rest;
            split_first(line, name, sizeof name, &rest);
            upper_inplace(name);
            int idx = -1;
            for (int k = 0; k < nm; k++) if (!strcmp(mnt[k].name, name)) idx = k;
            if (name[0] && idx >= 0) {
                char actual[MAX_PARAMS][128]; int na = 0;
                if (*rest) na = split_commas(rest, actual, MAX_PARAMS);
                /* dict(zip(params, actual)) */
                char keys[MAX_PARAMS][128], vals[MAX_PARAMS][128]; int nk = 0;
                int z = mnt[idx].nparams < na ? mnt[idx].nparams : na;
                for (int k = 0; k < z; k++) {
                    int f = -1;
                    for (int q = 0; q < nk; q++) if (!strcmp(keys[q], mnt[idx].params[k])) f = q;
                    if (f < 0) { f = nk++; snprintf(keys[f], 128, "%s", mnt[idx].params[k]); }
                    snprintf(vals[f], 128, "%s", actual[k]);
                }
                for (int b = 0; b < mnt[idx].nbody; b++) {
                    char *cur = xstrdup(mnt[idx].body[b]);
                    for (int q = 0; q < nk; q++) {
                        char *r = replace_all(cur, keys[q], vals[q]);
                        free(cur); cur = r;
                    }
                    exp[ne++] = cur;
                }
            } else if (strcmp(up, "END") != 0) {
                exp[ne++] = xstrdup(line);
            }
        }
        i++;
    }

    printf("\nMACRO NAME TABLE\n");
    for (int k = 0; k < nm; k++) {
        printf("%s [", mnt[k].name);
        for (int q = 0; q < mnt[k].nparams; q++) printf("%s'%s'", q ? ", " : "", mnt[k].params[q]);
        printf("]\n");
    }

    size_t total = 1;
    for (int k = 0; k < ne; k++) total += strlen(exp[k]) + 1;
    char *res = (char *)malloc(total); res[0] = 0;
    for (int k = 0; k < ne; k++) { if (k) strcat(res, "\n"); strcat(res, exp[k]); }

    free_lines(exp, ne); free(exp);
    for (int k = 0; k < nm; k++) { free_lines(mnt[k].body, mnt[k].nbody); free(mnt[k].body); }
    return res;
}
