#include <stdio.h>
#include <ctype.h>
#include <wchar.h>
#include <stdlib.h>
#include <string.h>
#include "quote.h"

void print_quoted(const char *s, QuotingStyle style, int hide_control, int show_controls, int literal_names) {
    if (literal_names) {
        fputs(s, stdout);
        return;
    }
    int quote = (style == QUOTE_C);
    int escape_nonprint = (style == QUOTE_C || style == QUOTE_ESCAPE);
    if (show_controls) {
        hide_control = 0;
        escape_nonprint = 0;
    }
    if (!quote && !escape_nonprint && !hide_control) {
        fputs(s, stdout);
        return;
    }
    if (quote)
        putchar('"');
    mbstate_t st;
    memset(&st, 0, sizeof(st));
    const char *p = s;
    while (*p) {
        wchar_t wc;
        size_t n = mbrtowc(&wc, p, MB_CUR_MAX, &st);
        if (n == (size_t)-1 || n == (size_t)-2) {
            wc = (unsigned char)*p;
            n = 1;
            memset(&st, 0, sizeof(st));
        }
        if (quote && (wc == L'"' || wc == L'\\'))
            putchar('\\');
        int w = wcwidth(wc);
        if (w < 0) {
            if (hide_control) {
                putchar('?');
            } else if (escape_nonprint) {
                for (size_t i = 0; i < n; i++) {
                    printf("\\%03o", (unsigned char)p[i]);
                }
            } else {
                fwrite(p, 1, n, stdout);
            }
        } else {
            fwrite(p, 1, n, stdout);
        }
        p += n;
    }
    if (quote)
        putchar('"');
}
