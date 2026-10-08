#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include "toolkit.h"

#define MAX_SYMS 512
#define MAX_IC   1000
#define MAX_OPS  8
#define MAX_ERR  200
#define MAX_PARTS 64

typedef struct { const char *name; const char *code; } Pair;

static const Pair OPTAB[] = {
    {"STOP","00"},{"ADD","01"},{"SUB","02"},{"MULT","03"},{"MOVER","04"},{"MOVEM","05"},
    {"COMP","06"},{"BC","07"},{"DIV","08"},{"READ","09"},{"PRINT","10"}};
static const Pair DL[]  = {{"DC","01"},{"DS","02"}};
static const Pair AD[]  = {{"START","01"},{"END","02"},{"ORIGIN","03"},{"EQU","04"}};
static const Pair REG[] = {{"AREG","1"},{"BREG","2"},{"CREG","3"},{"DREG","4"}};

#define COUNT(t) ((int)(sizeof(t) / sizeof((t)[0])))

static const char *lookup(const Pair *t, int n, const char *k) {
    for (int i = 0; i < n; i++) if (strcmp(t[i].name, k) == 0) return t[i].code;
    return NULL;
}

typedef struct { char name[64]; long long value; } Symbol;
typedef struct { long long lc; char code[24]; char ops[MAX_OPS][128]; int nops; } ICEntry;
typedef struct { long long lc; char op[24]; char reg[8]; char addr[128]; int is_data; } ObjEntry;

typedef struct {
    Symbol  symbols[MAX_SYMS]; int nsyms;
    ICEntry ic[MAX_IC];        int nic;
    long long location_counter;
    char errors[MAX_ERR][160]; int nerrors;
} Asm;

typedef struct {
    char label[128]; int has_label;
    char opcode[128]; int has_opcode;
    char operands[MAX_PARTS][128]; int nops;
} ParsedLine;

/* ---------------- helpers ---------------- */
static void add_error(Asm *a, const char *fmt, ...) {
    if (a->nerrors >= MAX_ERR) return;
    va_list ap; va_start(ap, fmt);
    vsnprintf(a->errors[a->nerrors++], sizeof a->errors[0], fmt, ap);
    va_end(ap);
}

static int find_symbol(Asm *a, const char *name) {
    for (int i = 0; i < a->nsyms; i++) if (strcmp(a->symbols[i].name, name) == 0) return i;
    return -1;
}

static void set_symbol(Asm *a, const char *name, long long v) {
    int i = find_symbol(a, name);
    if (i < 0) {
        if (a->nsyms >= MAX_SYMS) return;
        i = a->nsyms++;
        snprintf(a->symbols[i].name, sizeof a->symbols[i].name, "%s", name);
    }
    a->symbols[i].value = v;
}

static ICEntry *new_ic(Asm *a, const char *code) {
    if (a->nic >= MAX_IC) return NULL;
    ICEntry *e = &a->ic[a->nic++];
    e->lc = a->location_counter;
    snprintf(e->code, sizeof e->code, "%s", code);
    e->nops = 0;
    return e;
}

static void ic_op(ICEntry *e, const char *fmt, ...) {
    if (!e || e->nops >= MAX_OPS) return;
    va_list ap; va_start(ap, fmt);
    vsnprintf(e->ops[e->nops++], sizeof e->ops[0], fmt, ap);
    va_end(ap);
}

static int all_digits(const char *s) {
    if (!*s) return 0;
    for (; *s; s++) if (!isdigit((unsigned char)*s)) return 0;
    return 1;
}

static void zfill3(long long v, char *out, size_t sz) {
    char tmp[40]; snprintf(tmp, sizeof tmp, "%lld", v);
    char sign = 0; const char *d = tmp;
    if (*d == '-' || *d == '+') { sign = *d; d++; }
    int len = (int)strlen(tmp);
    int pad = 3 - len; if (pad < 0) pad = 0;
    char buf[48]; int k = 0;
    if (sign) buf[k++] = sign;
    while (pad--) buf[k++] = '0';
    strcpy(buf + k, d);
    snprintf(out, sz, "%s", buf);
}

static void remove_all(char *s, const char *sub) {
    size_t m = strlen(sub); char *p;
    while ((p = strstr(s, sub)) != NULL) memmove(p, p + m, strlen(p + m) + 1);
}

/* ---------------- split source line ---------------- */
static void parse_line(const char *line, ParsedLine *pl) {
    char buf[MAX_LINE + 1];
    char parts[MAX_PARTS][128]; int np = 0;
    pl->has_label = pl->has_opcode = 0; pl->nops = 0;
    snprintf(buf, sizeof buf, "%s", line);
    trim(buf);
    if (!*buf) return;
    for (char *p = buf; *p; p++) if (*p == ',') *p = ' ';
    for (char *t = strtok(buf, " \t\r\n\f\v"); t && np < MAX_PARTS; t = strtok(NULL, " \t\r\n\f\v"))
        snprintf(parts[np++], 128, "%s", t);
    if (np == 0) return;

    char first[128]; snprintf(first, sizeof first, "%s", parts[0]); upper_inplace(first);
    int start;
    if (lookup(OPTAB, COUNT(OPTAB), first) || lookup(DL, COUNT(DL), first) || lookup(AD, COUNT(AD), first)) {
        snprintf(pl->opcode, sizeof pl->opcode, "%s", first); pl->has_opcode = 1;
        start = 1;
    } else {
        snprintf(pl->label, sizeof pl->label, "%s", first); pl->has_label = 1;
        if (np < 2) return;
        snprintf(pl->opcode, sizeof pl->opcode, "%s", parts[1]); upper_inplace(pl->opcode);
        pl->has_opcode = 1;
        start = 2;
    }
    for (int i = start; i < np; i++) snprintf(pl->operands[pl->nops++], 128, "%s", parts[i]);
}

/* ---------------- expression such as A+1, A-1, L2+1 ---------------- */
static int evaluate_expression(Asm *a, const char *expr, long long *out, char *err, size_t esz) {
    char e[MAX_LINE + 1]; int k = 0;
    for (const char *p = expr; *p && k < MAX_LINE; p++)
        if (*p != ' ') e[k++] = (char)toupper((unsigned char)*p);
    e[k] = 0;

    long long result = 0; int sign = 1, ntok = 0, i = 0;
    while (e[i]) {
        unsigned char c = (unsigned char)e[i];
        if (isalpha(c) || c == '_') {
            char tok[128]; int j = i + 1;
            while (isalnum((unsigned char)e[j]) || e[j] == '_') j++;
            snprintf(tok, sizeof tok, "%.*s", j - i, e + i);
            ntok++;
            int si = find_symbol(a, tok);
            if (si < 0) { snprintf(err, esz, "Undefined symbol '%s'", tok); return 0; }
            result += sign * a->symbols[si].value; sign = 1; i = j;
        } else if (isdigit(c)) {
            int j = i; long long v = 0;
            while (isdigit((unsigned char)e[j])) { v = v * 10 + (e[j] - '0'); j++; }
            ntok++; result += sign * v; sign = 1; i = j;
        } else if (c == '+') { sign = 1;  ntok++; i++; }
        else if (c == '-')   { sign = -1; ntok++; i++; }
        else i++;
    }
    if (ntok == 0) { snprintf(err, esz, "Invalid expression"); return 0; }
    *out = result;
    return 1;
}

/* ---------------- PASS 1 ---------------- */
static void pass1(Asm *a, char **src, int n) {
    a->nsyms = a->nic = a->nerrors = 0;
    char err[160];

    for (int ln = 1; ln <= n; ln++) {
        ParsedLine p; parse_line(src[ln - 1], &p);
        if (!p.has_opcode) continue;
        const char *opcode = p.opcode;
        const char *label = p.has_label ? p.label : NULL;
        long long v;

        if (!strcmp(opcode, "START")) {
            if (p.nops > 0 && parse_int(p.operands[0], &v)) a->location_counter = v;
            else add_error(a, "Line %d: Invalid START", ln);
            ICEntry *e = new_ic(a, "(AD,01)");
            ic_op(e, "(C,%s)", p.nops > 0 ? p.operands[0] : "");
            continue;
        }

        if (!strcmp(opcode, "EQU")) {
            if (!label) { add_error(a, "Line %d: EQU requires label", ln); continue; }
            char joined[MAX_LINE + 1] = "";
            for (int i = 0; i < p.nops; i++) strncat(joined, p.operands[i], MAX_LINE - strlen(joined));
            if (evaluate_expression(a, joined, &v, err, sizeof err)) {
                set_symbol(a, label, v);
                ICEntry *e = new_ic(a, "(AD,04)");
                ic_op(e, "(S,%s)", label);
                ic_op(e, "(C,%lld)", v);
            } else add_error(a, "Line %d: %s", ln, err);
            continue;
        }

        if (!strcmp(opcode, "ORIGIN")) {
            char joined[MAX_LINE + 1] = "";
            for (int i = 0; i < p.nops; i++) strncat(joined, p.operands[i], MAX_LINE - strlen(joined));
            if (evaluate_expression(a, joined, &v, err, sizeof err)) {
                ICEntry *e = new_ic(a, "(AD,03)");
                ic_op(e, "(C,%lld)", v);
                a->location_counter = v;
            } else add_error(a, "Line %d: %s", ln, err);
            continue;
        }

        if (!strcmp(opcode, "END")) {
            ICEntry *e = new_ic(a, "(AD,02)");
            if (p.nops > 0) {
                char o[128]; snprintf(o, sizeof o, "%s", p.operands[0]); upper_inplace(o);
                ic_op(e, "(S,%s)", o);
            } else ic_op(e, "%s", "");
            break;
        }

        if (label) set_symbol(a, label, a->location_counter);

        if (!strcmp(opcode, "DS")) {
            if (p.nops > 0 && parse_int(p.operands[0], &v)) {
                ICEntry *e = new_ic(a, "(DL,02)");
                ic_op(e, "(C,%lld)", v);
                a->location_counter += v;
            } else add_error(a, "Line %d: Invalid DS", ln);
        } else if (!strcmp(opcode, "DC")) {
            if (p.nops > 0) {
                char val[128]; snprintf(val, sizeof val, "%s", p.operands[0]);
                int s = 0, e2 = (int)strlen(val);
                while (s < e2 && val[s] == '"') s++;
                while (e2 > s && val[e2 - 1] == '"') e2--;
                val[e2] = 0;
                ICEntry *e = new_ic(a, "(DL,01)");
                ic_op(e, "(C,%s)", val + s);
                a->location_counter += 1;
            } else add_error(a, "Line %d: Invalid DC", ln);
        } else if (lookup(OPTAB, COUNT(OPTAB), opcode)) {
            char code[32]; snprintf(code, sizeof code, "(IS,%s)", lookup(OPTAB, COUNT(OPTAB), opcode));
            ICEntry *e = new_ic(a, code);
            for (int i = 0; i < p.nops; i++) {
                char o[128]; snprintf(o, sizeof o, "%s", p.operands[i]); upper_inplace(o);
                const char *r = lookup(REG, COUNT(REG), o);
                if (r) ic_op(e, "(%s)", r);
                else if (find_symbol(a, o) >= 0) ic_op(e, "(S,%s)", o);
                else if (all_digits(o)) ic_op(e, "(C,%s)", o);
                else ic_op(e, "(S,%s)", o);   /* forward reference */
            }
            a->location_counter += 1;
        } else {
            add_error(a, "Line %d: Unknown opcode '%s'", ln, opcode);
        }
    }
}

/* ---------------- PASS 2 ---------------- */
static int pass2(Asm *a, ObjEntry *obj) {
    int n = 0;
    for (int k = 0; k < a->nic; k++) {
        ICEntry *it = &a->ic[k];
        if (!strncmp(it->code, "(AD", 3)) continue;

        if (!strcmp(it->code, "(DL,02)") || !strcmp(it->code, "(DL,01)")) {
            char val[128];
            snprintf(val, sizeof val, "%s", it->nops > 0 ? it->ops[0] : "");
            remove_all(val, "(C,"); remove_all(val, ")");
            ObjEntry *o = &obj[n++];
            o->lc = it->lc; o->is_data = 1; o->reg[0] = 0;
            if (!strcmp(it->code, "(DL,02)")) { snprintf(o->op, sizeof o->op, "DS"); snprintf(o->addr, sizeof o->addr, "RESERVE %s", val); }
            else { snprintf(o->op, sizeof o->op, "DC"); snprintf(o->addr, sizeof o->addr, "%s", val); }
            continue;
        }

        if (strncmp(it->code, "(IS,", 4) != 0) continue;
        char machine[24]; snprintf(machine, sizeof machine, "%s", it->code + 4);
        char *cp = strchr(machine, ')'); if (cp) *cp = 0;

        char reg[8] = "0", address[64] = "000";
        for (int i = 0; i < it->nops; i++) {
            const char *op = it->ops[i];
            if (strlen(op) == 3 && op[0] == '(' && isdigit((unsigned char)op[1]) && op[2] == ')') {
                reg[0] = op[1]; reg[1] = 0;
            } else if (!strncmp(op, "(S,", 3)) {
                char sym[128]; snprintf(sym, sizeof sym, "%s", op + 3);
                size_t L = strlen(sym); if (L) sym[L - 1] = 0;
                int si = find_symbol(a, sym);
                if (si >= 0) zfill3(a->symbols[si].value, address, sizeof address);
            } else if (!strncmp(op, "(C,", 3)) {
                char cv[128]; snprintf(cv, sizeof cv, "%s", op + 3);
                size_t L = strlen(cv); if (L) cv[L - 1] = 0;
                long long v;
                if (parse_int(cv, &v)) zfill3(v, address, sizeof address);
            }
        }
        ObjEntry *o = &obj[n++];
        o->lc = it->lc; o->is_data = 0;
        snprintf(o->op, sizeof o->op, "%s", machine);
        snprintf(o->reg, sizeof o->reg, "%s", reg);
        snprintf(o->addr, sizeof o->addr, "%s", address);
    }
    return n;
}

/* ---------------- DISPLAY ---------------- */
static void rule(char c) { for (int i = 0; i < 75; i++) putchar(c); putchar('\n'); }

static void display(Asm *a) {
    printf("\n"); rule('=');
    printf("                    SYMBOL TABLE\n");
    rule('=');
    printf("%-15s%-15s\n", "SYMBOL", "ADDRESS");
    rule('-');
    for (int i = 0; i < a->nsyms; i++)
        printf("%-15s%-15lld\n", a->symbols[i].name, a->symbols[i].value);

    printf("\n"); rule('=');
    printf("                  INTERMEDIATE CODE\n");
    rule('=');
    printf("%-8s%-15s%-18s%-18s\n", "LC", "OPCODE", "OPERAND 1", "OPERAND 2");
    rule('-');
    for (int i = 0; i < a->nic; i++) {
        ICEntry *it = &a->ic[i];
        printf("%-8lld%-15s%-18s%-18s\n", it->lc, it->code,
               it->nops > 0 ? it->ops[0] : "", it->nops > 1 ? it->ops[1] : "");
    }

    printf("\n"); rule('=');
    printf("                     OBJECT CODE\n");
    rule('=');
    printf("%-8s%-12s%-12s%-18s\n", "LC", "OPCODE", "REGISTER", "ADDRESS / DATA");
    rule('-');
    ObjEntry *obj = (ObjEntry *)malloc(sizeof(ObjEntry) * MAX_IC);
    int n = pass2(a, obj);
    for (int i = 0; i < n; i++) {
        if (obj[i].is_data)
            printf("%-8lld%-12s%-12s%-18s\n", obj[i].lc, obj[i].op, "-", obj[i].addr);
        else
            printf("%-8lld%-12s%-12s%-18s\n", obj[i].lc, obj[i].op, obj[i].reg, obj[i].addr);
    }
    free(obj);

    if (a->nerrors) {
        printf("\n"); rule('=');
        printf("                       ERRORS\n");
        rule('=');
        for (int i = 0; i < a->nerrors; i++) printf("%s\n", a->errors[i]);
    } else {
        printf("\nAssembly completed successfully.\n");
    }
}

void assembler_run(char **lines, int n) {
    Asm *a = (Asm *)calloc(1, sizeof(Asm));
    if (!a) { fprintf(stderr, "Out of memory\n"); exit(1); }
    pass1(a, lines, n);
    display(a);
    free(a);
}
