#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "toolkit.h"

static char *read_all(void) {
    size_t cap = 8192, n = 0;
    char *buf = malloc(cap);
    if (!buf) return NULL;
    int c;
    while ((c = getchar()) != EOF) {
        if (n + 1 >= cap) { cap *= 2; char *p = realloc(buf, cap); if (!p) { free(buf); return NULL; } buf = p; }
        buf[n++] = (char)c;
    }
    buf[n] = 0;
    return buf;
}

static char **split_lines(char *s, int *out_n) {
    int cap = 64, n = 0;
    char **lines = malloc(sizeof(char*) * cap);
    char *p = s;
    while (*p) {
        char *start = p;
        while (*p && *p != '\n') p++;
        if (*p) *p++ = 0;
        if (p > start && p[-1] == '\r') p[-1] = 0;
        if (n >= cap) { cap *= 2; lines = realloc(lines, sizeof(char*) * cap); }
        lines[n++] = start;
    }
    if (n == 0) { lines[n++] = s; }
    *out_n = n;
    return lines;
}

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "mode required\n"); return 2; }
    char *input = read_all();
    if (!input) return 3;
    const char *mode = argv[1];

    if (!strcmp(mode, "lexical")) {
        Token toks[MAX_LINE + 1];
        int n = lexical_analyzer(input, toks, MAX_LINE);
        for (int i = 0; i < n; i++)
            printf("LEXICAL|%s|%.*s|%d\n", toks[i].type, toks[i].len, input + toks[i].start, toks[i].start);
    } else if (!strcmp(mode, "symbol")) {
        SymbolTable t; symtab_init(&t); symtab_build_from_text(&t, input);
        for (int i = 0; i < t.count; i++)
            printf("SYMBOL|%s|%s|%d|%d\n", t.entries[i].name, t.entries[i].datatype, t.entries[i].size, t.entries[i].address);
    } else if (!strcmp(mode, "parser")) {
        printf("STATUS|%s\n", rd_parse(input) ? "VALID" : "INVALID");
    } else if (!strcmp(mode, "quadruple")) {
        Quad q[MAX_LINE + 1]; char err[256]; int cnt = 0;
        if (generate_quadruples(input, q, &cnt, err, sizeof err)) {
            for (int i = 0; i < cnt; i++) printf("QUAD|%d|%s|%s|%s|%s\n", i + 1, q[i].op, q[i].a, q[i].b, q[i].r);
        } else printf("ERROR|%s\n", err);
    } else {
        int n = 0; char **lines = split_lines(input, &n);
        if (!strcmp(mode, "assembler")) {
            assembler_run(lines, n);
        } else if (!strcmp(mode, "macro")) {
            char *res = macro_process(lines, n);
            printf("\nEXPANDED PROGRAM\n%s\n", res ? res : "");
            free(res);
        } else if (!strcmp(mode, "linker")) {
            linker_loader_run(lines, n);
        } else if (!strcmp(mode, "optimizer")) {
            int on = 0; char **out = optimize_code(lines, n, &on);
            for (int i = 0; i < on; i++) printf("%s%s", i ? "\n" : "", out[i]);
            printf("\n");
            free_lines(out, on); free(out);
        } else {
            fprintf(stderr, "unknown mode\n"); free(lines); free(input); return 2;
        }
        free(lines);
    }
    free(input);
    return 0;
}
