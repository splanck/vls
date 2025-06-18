#include "color.h"

const char *color_reset(void) { return "\x1b[0m"; }
const char *color_dir(void)   { return "\x1b[1;34m"; }
const char *color_link(void)  { return "\x1b[1;36m"; }
const char *color_exec(void)  { return "\x1b[1;32m"; }
