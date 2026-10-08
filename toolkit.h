#ifndef TOOLKIT_H
#define TOOLKIT_H

#define MAX_LINE  1024
#define MAX_LINES 1000

/* ---- util.c ---- */
int   read_line(char *buf, int size);          /* returns 0 on EOF */
void  trim(char *s);                           /* strip whitespace in place */
void  upper_inplace(char *s);
int   parse_int(const char *s, long long *out);/* like Python int(); 1 = ok */
char *xstrdup(const char *s);
void  free_lines(char **lines, int n);

/* ---- lexical_analyzer.c ---- */
typedef struct { const char *type; int start; int len; } Token;
int lexical_analyzer(const char *src, Token *out, int max);

/* ---- symbol_table.c ---- */
#define ST_MAX 256
typedef struct { char name[64]; char datatype[16]; int size; int address; } SymbolEntry;
typedef struct { SymbolEntry entries[ST_MAX]; int count; int next_address; } SymbolTable;
void         symtab_init(SymbolTable *t);
void         symtab_insert(SymbolTable *t, const char *name, const char *datatype, int size);
void         symtab_build_from_text(SymbolTable *t, const char *text);
SymbolEntry *symtab_lookup(SymbolTable *t, const char *name);
void         symtab_display(const SymbolTable *t);

/* ---- assembler.c ---- */
void assembler_run(char **lines, int n);

/* ---- macro_processor.c ---- */
char *macro_process(char **lines, int n);      /* prints MNT, returns expanded text (malloc'd) */

/* ---- linker_loader.c ---- */
void linker_loader_run(char **records, int n);

/* ---- recursive_descent_parser.c ---- */
int rd_parse(const char *text);                /* 1 = VALID, 0 = INVALID */

/* ---- quadruple.c ---- */
typedef struct { char op[2]; char a[128]; char b[128]; char r[16]; } Quad;
int generate_quadruples(const char *expr, Quad *out, int *count, char *err, int errsz);

/* ---- optimizer.c ---- */
char **optimize_code(char **lines, int n, int *out_n);

#endif
