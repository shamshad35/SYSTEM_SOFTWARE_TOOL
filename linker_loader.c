#include <stdio.h>
#include <string.h>
#include "toolkit.h"

#define LL_BASE 1000
#define LL_MODULE_SIZE 100
#define LL_MAX 1000

void linker_loader_run(char **records, int n) {
    static char names[LL_MAX][128];
    static long long addrs[LL_MAX];
    int nt = 0;
    long long base = LL_BASE;

    for (int r = 0; r < n; r++) {
        char buf[MAX_LINE + 1];
        snprintf(buf, sizeof buf, "%s", records[r]);
        char *tok = strtok(buf, " \t\r\n\f\v");
        if (!tok) continue;
        printf("Module %s -> base %lld\n", tok, base);
        while ((tok = strtok(NULL, " \t\r\n\f\v")) != NULL) {
            char *eq = strchr(tok, '=');
            if (!eq) continue;
            *eq = 0;
            long long a;
            if (strchr(eq + 1, '=') || !parse_int(eq + 1, &a)) {
                printf("Invalid entry '%s=%s' skipped\n", tok, eq + 1);
                continue;
            }
            int idx = -1;
            for (int i = 0; i < nt; i++) if (!strcmp(names[i], tok)) idx = i;
            if (idx < 0 && nt < LL_MAX) { idx = nt++; snprintf(names[idx], 128, "%s", tok); }
            if (idx >= 0) addrs[idx] = base + a;
        }
        base += LL_MODULE_SIZE;
    }
    printf("\nABSOLUTE SYMBOL TABLE\n");
    for (int i = 0; i < nt; i++) printf("%-15s%lld\n", names[i], addrs[i]);
}
