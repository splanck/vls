#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "args.h"

void parse_args(int argc, char *argv[], Args *args) {
    args->use_color = 1;
    args->show_hidden = 0;
    args->long_format = 0;
    args->reverse = 0;
    args->path = ".";

    static struct option long_options[] = {
        {"no-color", no_argument, 0, 'C'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "alrCh", long_options, NULL)) != -1) {
        switch (opt) {
        case 'a':
            args->show_hidden = 1;
            break;
        case 'l':
            args->long_format = 1;
            break;
        case 'r':
            args->reverse = 1;
            break;
        case 'C':
            args->use_color = 0;
            break;
        case 'h':
            printf("Usage: %s [-a] [-l] [-r] [--no-color] [path]\n", argv[0]);
            exit(0);
            break;
        default:
            fprintf(stderr, "Usage: %s [-a] [-l] [-r] [--no-color] [path]\n", argv[0]);
            exit(1);
        }
    }

    if (optind < argc)
        args->path = argv[optind];
}
