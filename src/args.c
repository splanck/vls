#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "args.h"
#include <string.h>

void parse_args(int argc, char *argv[], Args *args) {
    args->use_color = 1;
    args->show_hidden = 0;
    args->almost_all = 0;
    args->long_format = 0;
    args->show_inode = 0;
    args->sort_time = 0;
    args->sort_size = 0;
    args->reverse = 0;
    args->recursive = 0;
    args->list_dirs_only = 0;
    args->classify = 0;
    args->follow_links = 0;
    args->human_readable = 0;
    args->paths = NULL;
    args->path_count = 0;

    static struct option long_options[] = {
        {"no-color", no_argument, 0, 'C'},
        {"almost-all", no_argument, 0, 'A'},
        {"help", no_argument, 0, 1},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "AialtrSChRFhLd", long_options, NULL)) != -1) {
        switch (opt) {
        case 'A':
            args->almost_all = 1;
            break;
        case 'a':
            args->show_hidden = 1;
            break;
        case 'l':
            args->long_format = 1;
            break;
        case 'i':
            args->show_inode = 1;
            break;
        case 't':
            args->sort_time = 1;
            break;
        case 'S':
            args->sort_size = 1;
            break;
        case 'r':
            args->reverse = 1;
            break;
        case 'R':
            args->recursive = 1;
            break;
        case 'd':
            args->list_dirs_only = 1;
            break;
        case 'F':
            args->classify = 1;
            break;
        case 'L':
            args->follow_links = 1;
            break;
        case 'h':
            args->human_readable = 1;
            break;
        case 'C':
            args->use_color = 0;
            break;
        case 1:
            printf("Usage: %s [-a] [-A] [-l] [-i] [-t] [-S] [-r] [-R] [-d] [-L] [-F] [-h] [--no-color] [--almost-all] [--help] [path]\n", argv[0]);
            printf("Default is to display information about symbolic links. Use -L to follow them.\n");
            exit(0);
            break;
        default:
            fprintf(stderr, "Usage: %s [-a] [-A] [-l] [-i] [-t] [-S] [-r] [-R] [-d] [-L] [-F] [-h] [--no-color] [--almost-all] [--help] [path]\n", argv[0]);
            exit(1);
        }
    }

    if (optind < argc) {
        args->path_count = (size_t)(argc - optind);
        args->paths = malloc(args->path_count * sizeof(const char *));
        if (!args->paths) {
            perror("malloc");
            exit(1);
        }
        for (size_t i = 0; i < args->path_count; i++)
            args->paths[i] = argv[optind + i];
    } else {
        args->path_count = 1;
        args->paths = malloc(sizeof(const char *));
        if (!args->paths) {
            perror("malloc");
            exit(1);
        }
        args->paths[0] = ".";
    }
}
