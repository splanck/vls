#include <stdio.h>
#include "list.h"

#define VLS_VERSION "0.1"

int main(void) {
    printf("vls %s\n", VLS_VERSION);
    list_directory(".");
    return 0;
}

