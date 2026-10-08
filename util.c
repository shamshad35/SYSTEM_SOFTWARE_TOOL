#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "toolkit.h"

int read_line(char *buf, int size) {
    if (!fgets(buf, size, stdin)) return 0;
    size_t n = strlen(buf);
    if (n > 0 && buf[n - 1] == '\n') buf[--n] = 0;
    else if (n == (size_t)size - 1) { int c; while ((c = getchar()) != '\n' && c != EOF); }
    if (n > 0 && buf[n - 1] == '\r') buf[--n] = 0;
    return 1;
}

void trim(char *s) {
    size_t n = strlen(s), a = 0, b = n;
    while (a < n && isspace((unsigned char)s[a])) a++;
    while (b > a && isspace((unsigned char)s[b - 1])) b--;
    memmove(s, s + a, b - a);
    s[b - a] = 0;
}

void upper_inplace(char *s) {
    for (; *s; s++) *s = (char)toupper((unsigned char)*s);
}

int parse_int(const char *s, long long *out) {
    const char *p = s;
    int neg = 0;
    while (isspace((unsigned char)*p)) p++;
    if (*p == '+' || *p == '-') { neg = (*p == '-'); p++; }
    if (!isdigit((unsigned char)*p)) return 0;
    long long v = 0;
    while (isdigit((unsigned char)*p)) { v = v * 10 + (*p - '0'); p++; }
    while (isspace((unsigned char)*p)) p++;
    if (*p) return 0;
    *out = neg ? -v : v;
    return 1;
}

char *xstrdup(const char *s) {
    size_t n = strlen(s) + 1;
    char *p = (char *)malloc(n);
    if (!p) { fprintf(stderr, "Out of memory\n"); exit(1); }
    memcpy(p, s, n);
    return p;
}

void free_lines(char **lines, int n) {
    for (int i = 0; i < n; i++) free(lines[i]);
}
