#include <stdio.h>
#include <ctype.h>
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
    for (const char *p = s; *p; p++) {
        unsigned char c = (unsigned char)*p;
        if (quote && (c == '"' || c == '\\'))
            putchar('\\');
        if (!isprint(c)) {
            if (hide_control) {
                putchar('?');
            } else if (escape_nonprint) {
                char buf[5];
                snprintf(buf, sizeof(buf), "\\%03o", c);
                fputs(buf, stdout);
            } else {
                putchar(c);
            }
        } else {
            putchar(c);
        }
    }
    if (quote)
        putchar('"');
}
