#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

char *join_path(const char *dir, const char *name) {
    char *result = NULL;
    if (asprintf(&result, "%s/%s", dir, name) < 0)
        return NULL;
    return result;
}
