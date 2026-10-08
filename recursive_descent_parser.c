#include <string.h>
#include <ctype.h>
#include "toolkit.h"

/* token types: operators/parens use their own char value */
enum { T_ID = 256, T_NUM, T_INVALID };

typedef struct { int tok[MAX_LINE + 1]; int n; int pos; } RD;

static int cur(RD *p) { return p->pos < p->n ? p->tok[p->pos] : -1; }
static int eat(RD *p, int t) { if (cur(p) == t) { p->pos++; return 1; } return 0; }

static int E(RD *p);
static int F(RD *p) {
    if (cur(p) == T_ID || cur(p) == T_NUM) { p->pos++; return 1; }
    if (eat(p, '(')) { if (E(p) && eat(p, ')')) return 1; }
    return 0;
}
static int T(RD *p) {
    if (!F(p)) return 0;
    while (cur(p) == '*' || cur(p) == '/') { p->pos++; if (!F(p)) return 0; }
    return 1;
}
static int E(RD *p) {
    if (!T(p)) return 0;
    while (cur(p) == '+' || cur(p) == '-') { p->pos++; if (!T(p)) return 0; }
    return 1;
}

int rd_parse(const char *text) {
    static RD p;
    int i = 0, len = (int)strlen(text);
    p.n = 0; p.pos = 0;
    while (i < len && p.n < MAX_LINE) {
        unsigned char c = (unsigned char)text[i];
        if (isspace(c)) i++;
        else if (isalpha(c) || c == '_') {
            int j = i + 1;
            while (isalnum((unsigned char)text[j]) || text[j] == '_') j++;
            p.tok[p.n++] = T_ID; i = j;
        } else if (isdigit(c)) {
            int j = i + 1;
            while (isdigit((unsigned char)text[j])) j++;
            p.tok[p.n++] = T_NUM; i = j;
        } else if (strchr("+-*/()", c)) { p.tok[p.n++] = c; i++; }
        else { p.tok[p.n++] = T_INVALID; i++; }
    }
    return E(&p) && cur(&p) == -1;
}
