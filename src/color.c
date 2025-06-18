#include "color.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static char reset_str[32] = "\x1b[0m";
static char dir_str[32]   = "\x1b[1;34m";
static char link_str[32]  = "\x1b[1;36m";
static char exec_str[32]  = "\x1b[1;32m";

static void set_color(char *buf, size_t sz, const char *code) {
    snprintf(buf, sz, "\x1b[%sm", code);
}

void color_init(void) {
    const char *env = getenv("LS_COLORS");
    if (!env)
        return;
    char *tmp = strdup(env);
    if (!tmp)
        return;
    char *tok = strtok(tmp, ":");
    while (tok) {
        if (strncmp(tok, "di=", 3) == 0)
            set_color(dir_str, sizeof(dir_str), tok + 3);
        else if (strncmp(tok, "ln=", 3) == 0)
            set_color(link_str, sizeof(link_str), tok + 3);
        else if (strncmp(tok, "ex=", 3) == 0)
            set_color(exec_str, sizeof(exec_str), tok + 3);
        else if (strncmp(tok, "rs=", 3) == 0)
            set_color(reset_str, sizeof(reset_str), tok + 3);
        tok = strtok(NULL, ":");
    }
    free(tmp);
}

const char *color_reset(void) { return reset_str; }
const char *color_dir(void)   { return dir_str; }
const char *color_link(void)  { return link_str; }
const char *color_exec(void)  { return exec_str; }
