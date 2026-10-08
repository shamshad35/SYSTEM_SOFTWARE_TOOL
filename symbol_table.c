#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "toolkit.h"

void symtab_init(SymbolTable *t) { t->count = 0; t->next_address = 1000; }

SymbolEntry *symtab_lookup(SymbolTable *t, const char *name) {
    for (int i = 0; i < t->count; i++)
        if (strcmp(t->entries[i].name, name) == 0) return &t->entries[i];
    return NULL;
}

void symtab_insert(SymbolTable *t, const char *name, const char *datatype, int size) {
    if (symtab_lookup(t, name) || t->count >= ST_MAX) return;
    SymbolEntry *e = &t->entries[t->count++];
    snprintf(e->name, sizeof e->name, "%s", name);
    snprintf(e->datatype, sizeof e->datatype, "%s", datatype);
    e->size = size;
    e->address = t->next_address;
    t->next_address += size;
}

static int type_size(const char *x) {
    if (!strcmp(x, "int") || !strcmp(x, "float")) return 4;
    if (!strcmp(x, "char")) return 1;
    if (!strcmp(x, "double")) return 8;
    return 0;
}

void symtab_build_from_text(SymbolTable *t, const char *text) {
    char buf[MAX_LINE + 1];
    char datatype[16] = "unknown";
    int cur_size = 1;
    snprintf(buf, sizeof buf, "%s", text);
    for (char *p = buf; *p; p++) if (*p == ',') *p = ' ';
    for (char *tok = strtok(buf, " \t\r\n\f\v"); tok; tok = strtok(NULL, " \t\r\n\f\v")) {
        int sz = type_size(tok);
        if (sz) { snprintf(datatype, sizeof datatype, "%s", tok); cur_size = sz; }
        else symtab_insert(t, tok, datatype, cur_size);
    }
}

void symtab_display(const SymbolTable *t) {
    printf("\nSYMBOL TABLE\n");
    for (int i = 0; i < 55; i++) putchar('-');
    printf("\n%-15s%-15s%-10s%-10s\n", "Name", "Datatype", "Size", "Address");
    for (int i = 0; i < t->count; i++) {
        const SymbolEntry *x = &t->entries[i];
        printf("%-15s%-15s%-10d%d\n", x->name, x->datatype, x->size, x->address);
    }
}
