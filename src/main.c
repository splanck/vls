#include <stdio.h>
#include <string.h>
#include "list.h"

#define VLS_VERSION "0.1"

int main(int argc, char *argv[]) {
    int use_color = 1;
    const char *path = ".";
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--no-color") == 0) {
            use_color = 0;
        } else {
            path = argv[i];
        }
    }

    printf("vls %s\n", VLS_VERSION);
    list_directory(path, use_color);
    return 0;
}

