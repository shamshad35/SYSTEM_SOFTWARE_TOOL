#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "toolkit.h"

#define TOKLEN 128

typedef struct {
    char vals[MAX_LINE + 1][TOKLEN]; int nv;
    char ops[MAX_LINE + 1]; int nop;
    Quad *out; int nout;
    int n;           /* temp counter */
    char *err; int errsz;
} Ctx;

static int prec(char c) { return (c == '+' || c == '-') ? 1 : 2; }

static int apply(Ctx *c) {
    char op = c->ops[--c->nop];
    if (c->nv < 2) { snprintf(c->err, c->errsz, "pop from empty list"); return 0; }
    char b[TOKLEN], a[TOKLEN];
    snprintf(b, sizeof b, "%s", c->vals[--c->nv]);
    snprintf(a, sizeof a, "%s", c->vals[--c->nv]);
    Quad *q = &c->out[c->nout++];
    q->op[0] = op; q->op[1] = 0;
    snprintf(q->a, sizeof q->a, "%s", a);
    snprintf(q->b, sizeof q->b, "%s", b);
    snprintf(q->r, sizeof q->r, "t%d", c->n++);
    snprintf(c->vals[c->nv++], TOKLEN, "%s", q->r);
    return 1;
}

int generate_quadruples(const char *expr, Quad *out, int *count, char *err, int errsz) {
    static char toks[MAX_LINE + 1][TOKLEN];
    static Ctx c;
    int nt = 0, i = 0, len = (int)strlen(expr);

    while (i < len && nt < MAX_LINE) {
        unsigned char ch = (unsigned char)expr[i];
        int j;
        if (isalpha(ch) || ch == '_') {
            j = i + 1;
            while (isalnum((unsigned char)expr[j]) || expr[j] == '_') j++;
        } else if (isdigit(ch)) {
            j = i + 1;
            while (isdigit((unsigned char)expr[j])) j++;
        } else if (strchr("()+-*/", ch)) {
            j = i + 1;
        } else { i++; continue; }
        snprintf(toks[nt++], TOKLEN, "%.*s", j - i, expr + i);
        i = j;
    }

    c.nv = c.nop = c.nout = 0; c.n = 1; c.out = out; c.err = err; c.errsz = errsz;

    for (int k = 0; k < nt; k++) {
        const char *x = toks[k];
        if (isalnum((unsigned char)x[0]) || x[0] == '_') {
            snprintf(c.vals[c.nv++], TOKLEN, "%s", x);
        } else if (x[0] == '(') {
            c.ops[c.nop++] = '(';
        } else if (x[0] == ')') {
            while (c.nop && c.ops[c.nop - 1] != '(') if (!apply(&c)) return 0;
            if (!c.nop) { snprintf(err, errsz, "Mismatched parentheses"); return 0; }
            c.nop--;
        } else {
            while (c.nop && c.ops[c.nop - 1] != '(' && prec(c.ops[c.nop - 1]) >= prec(x[0]))
                if (!apply(&c)) return 0;
            c.ops[c.nop++] = x[0];
        }
    }
    while (c.nop) {
        if (c.ops[c.nop - 1] == '(') { snprintf(err, errsz, "Mismatched parentheses"); return 0; }
        if (!apply(&c)) return 0;
    }
    *count = c.nout;
    return 1;
}
