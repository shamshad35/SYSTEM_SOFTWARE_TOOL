#include <string.h>
#include <ctype.h>
#include "toolkit.h"

static const char *KEYWORDS[] = {"int","float","char","double","if","else","while",
    "for","return","void","main","break","continue"};

static int is_keyword(const char *s, int len) {
    for (size_t i = 0; i < sizeof(KEYWORDS) / sizeof(KEYWORDS[0]); i++)
        if ((int)strlen(KEYWORDS[i]) == len && strncmp(KEYWORDS[i], s, len) == 0) return 1;
    return 0;
}

int lexical_analyzer(const char *s, Token *out, int max) {
    int n = 0, i = 0, len = (int)strlen(s);
    while (i < len && n < max) {
        unsigned char c = (unsigned char)s[i];
        int j = i + 1;
        const char *type;
        if (isdigit(c)) {
            j = i;
            while (isdigit((unsigned char)s[j])) j++;
            if (s[j] == '.' && isdigit((unsigned char)s[j + 1])) {
                j++;
                while (isdigit((unsigned char)s[j])) j++;
            }
            type = "NUMBER";
        } else if (isalpha(c) || c == '_') {
            while (isalnum((unsigned char)s[j]) || s[j] == '_') j++;
            type = is_keyword(s + i, j - i) ? "KEYWORD" : "IDENTIFIER";
        } else if ((c == '=' && s[i+1] == '=') || (c == '!' && s[i+1] == '=') ||
                   (c == '<' && s[i+1] == '=') || (c == '>' && s[i+1] == '=') ||
                   (c == '+' && s[i+1] == '+') || (c == '-' && s[i+1] == '-') ||
                   (c == '&' && s[i+1] == '&') || (c == '|' && s[i+1] == '|')) {
            j = i + 2; type = "OPERATOR";
        } else if (c && strchr("+-*/%=<>", c)) {
            type = "OPERATOR";
        } else if (c && strchr("()[]{};,:", c)) {
            type = "SEPARATOR";
        } else if (c == '"' || c == '\'') {
            const char *e = strchr(s + i + 1, c);
            if (e) { j = (int)(e - s) + 1; type = "STRING"; }
            else   { type = "UNKNOWN"; }
        } else if (isspace(c)) {
            while (isspace((unsigned char)s[j])) j++;
            i = j;
            continue;
        } else {
            type = "UNKNOWN";
        }
        out[n].type = type; out[n].start = i; out[n].len = j - i; n++;
        i = j;
    }
    return n;
}
