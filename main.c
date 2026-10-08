#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "toolkit.h"

static void pause_prompt(void) {
    char tmp[MAX_LINE];
    printf("\nPress Enter to continue...");
    fflush(stdout);
    read_line(tmp, sizeof tmp);
}

static int prompt(const char *msg, char *buf, int size) {
    printf("%s", msg);
    fflush(stdout);
    return read_line(buf, size);
}

/* mode 0: keep line, stop when it STARTS with END   (assembler)
   mode 1: keep line, stop when line == END          (macro)
   mode 2: drop END line, stop when line == END      (linker, optimizer) */
static int read_block(char **lines, int mode) {
    int n = 0;
    char buf[MAX_LINE];
    while (n < MAX_LINES && read_line(buf, sizeof buf)) {
        char t[MAX_LINE];
        snprintf(t, sizeof t, "%s", buf); trim(t); upper_inplace(t);
        if (mode == 0) {
            lines[n++] = xstrdup(buf);
            if (strncmp(t, "END", 3) == 0) break;
        } else if (mode == 1) {
            lines[n++] = xstrdup(buf);
            if (!strcmp(t, "END")) break;
        } else {
            if (!strcmp(t, "END")) break;
            lines[n++] = xstrdup(buf);
        }
    }
    return n;
}

int main(void) {
    char c[MAX_LINE], s[MAX_LINE];
    static char *lines[MAX_LINES];

    for (;;) {
        printf("\n");
        for (int i = 0; i < 55; i++) putchar('=');
        printf("\n MINI SYSTEM SOFTWARE TOOLKIT\n");
        for (int i = 0; i < 55; i++) putchar('=');
        printf("\n1. Lexical Analyzer\n2. Symbol Table\n3. Two-Pass Assembler\n");
        printf("4. Macro Processor\n5. Linker & Loader\n6. Recursive Descent Parser\n");
        printf("7. Quadruple Generator\n8. Code Optimizer\n9. Exit\n");
        if (!prompt("Enter choice: ", c, sizeof c)) break;
        trim(c);

        if (!strcmp(c, "1")) {
            if (!prompt("Enter source code: ", s, sizeof s)) break;
            static Token toks[MAX_LINE + 1];
            int n = lexical_analyzer(s, toks, MAX_LINE);
            for (int i = 0; i < n; i++) {
                printf("%-15s %.*s", toks[i].type, toks[i].len, s + toks[i].start);
                for (int p = toks[i].len; p < 15; p++) putchar(' ');
                printf(" position %d\n", toks[i].start);
            }
            pause_prompt();
        } else if (!strcmp(c, "2")) {
            if (!prompt("Example: int x y float total\nInput: ", s, sizeof s)) break;
            SymbolTable *t = (SymbolTable *)malloc(sizeof(SymbolTable));
            symtab_init(t); symtab_build_from_text(t, s); symtab_display(t);
            free(t);
            pause_prompt();
        } else if (!strcmp(c, "3")) {
            printf("\nEnter/paste the complete assembly program.\n");
            printf("Type END or END <operand> on the final line.\n");
            for (int i = 0; i < 60; i++) putchar('-');
            printf("\n");
            int n = read_block(lines, 0);
            assembler_run(lines, n);
            free_lines(lines, n);
            pause_prompt();
        } else if (!strcmp(c, "4")) {
            printf("Enter macro program; type END:\n");
            int n = read_block(lines, 1);
            char *res = macro_process(lines, n);
            printf("\nEXPANDED PROGRAM\n%s\n", res);
            free(res); free_lines(lines, n);
            pause_prompt();
        } else if (!strcmp(c, "5")) {
            printf("Enter records like: MAIN X=0 Y=20; type END:\n");
            int n = read_block(lines, 2);
            linker_loader_run(lines, n);
            free_lines(lines, n);
            pause_prompt();
        } else if (!strcmp(c, "6")) {
            if (!prompt("Expression: ", s, sizeof s)) break;
            printf("%s\n", rd_parse(s) ? "VALID" : "INVALID");
            pause_prompt();
        } else if (!strcmp(c, "7")) {
            if (!prompt("Expression: ", s, sizeof s)) break;
            static Quad q[MAX_LINE + 1];
            char err[128]; int cnt = 0;
            if (generate_quadruples(s, q, &cnt, err, sizeof err)) {
                printf("No.\tOp\tArg1\tArg2\tResult\n");
                for (int i = 0; i < cnt; i++)
                    printf("%d\t%s\t%s\t%s\t%s\n", i + 1, q[i].op, q[i].a, q[i].b, q[i].r);
            } else {
                printf("Error: %s\n", err);
            }
            pause_prompt();
        } else if (!strcmp(c, "8")) {
            printf("Enter three-address statements; type END:\n");
            int n = read_block(lines, 2), on = 0;
            char **out = optimize_code(lines, n, &on);
            for (int i = 0; i < on; i++) printf("%s%s", i ? "\n" : "", out[i]);
            printf("\n");
            free_lines(out, on); free(out); free_lines(lines, n);
            pause_prompt();
        } else if (!strcmp(c, "9")) {
            break;
        } else {
            printf("Invalid choice.\n");
        }
    }
    return 0;
}
