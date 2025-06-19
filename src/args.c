#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "args.h"
#include <string.h>
#include <unistd.h>
#include "version.h"

void parse_args(int argc, char *argv[], Args *args) {
    args->color_mode = COLOR_AUTO;
    args->show_hidden = 0;
    args->almost_all = 0;
    args->long_format = 0;
    args->show_inode = 0;
    args->sort_time = 0;
    args->sort_atime = 0;
    args->sort_ctime = 0;
    args->sort_size = 0;
    args->sort_extension = 0;
    args->unsorted = 0;
    args->reverse = 0;
    args->dirs_first = 0;
    args->recursive = 0;
    args->list_dirs_only = 0;
    args->classify = 0;
    args->slash_dirs = 0;
    args->follow_links = 0;
    args->deref_cmdline = 0;
    args->human_readable = 0;
    args->numeric_ids = 0;
    args->hide_owner = 0;
    args->hide_group = 0;
    args->show_context = 0;
    args->ignore_backups = 0;
    args->ignore_patterns = NULL;
    args->ignore_count = 0;
    args->columns = isatty(STDOUT_FILENO);
    args->across_columns = 0;
    args->one_per_line = 0;
    args->comma_separated = 0;
    args->show_blocks = 0;
    args->quote_names = 0;
    args->block_size = 0;
    args->paths = NULL;
    args->path_count = 0;

    static struct option long_options[] = {
        {"color", required_argument, 0, 2},
        {"almost-all", no_argument, 0, 'A'},
        {"ignore", required_argument, 0, 'I'},
        {"ignore-backups", no_argument, 0, 'B'},
        {"block-size", required_argument, 0, 3},
        {"group-directories-first", no_argument, 0, 4},
        {"quote-name", no_argument, 0, 'Q'},
        {"help", no_argument, 0, 1},
        {"version", no_argument, 0, 'V'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "AialtrucUfhXRFpI:BhHLZdgonCx1msQVk", long_options, NULL)) != -1) {
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
        case 'u':
            args->sort_atime = 1;
            break;
        case 'c':
            args->sort_ctime = 1;
            break;
        case 'S':
            args->sort_size = 1;
            break;
        case 'X':
            args->sort_extension = 1;
            break;
        case 'f':
        case 'U':
            args->unsorted = 1;
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
        case 'p':
            args->slash_dirs = 1;
            break;
        case 'I':
            args->ignore_patterns = realloc(args->ignore_patterns,
                                            (args->ignore_count + 1) * sizeof(char *));
            if (!args->ignore_patterns) {
                perror("realloc");
                exit(1);
            }
            args->ignore_patterns[args->ignore_count++] = optarg;
            break;
        case 'B':
            args->ignore_backups = 1;
            break;
        case 'F':
            args->classify = 1;
            break;
        case 'C':
            args->columns = 1;
            args->across_columns = 0;
            break;
        case 'x':
            args->columns = 1;
            args->across_columns = 1;
            break;
        case 'm':
            args->comma_separated = 1;
            break;
        case '1':
            args->one_per_line = 1;
            break;
        case 'L':
            args->follow_links = 1;
            break;
        case 'H':
            args->deref_cmdline = 1;
            break;
        case 'Z':
            args->show_context = 1;
            break;
        case 's':
            args->show_blocks = 1;
            break;
        case 'g':
            args->hide_owner = 1;
            break;
        case 'o':
            args->hide_group = 1;
            break;
        case 'h':
            args->human_readable = 1;
            break;
        case 'n':
            args->numeric_ids = 1;
            break;
        case 'Q':
            args->quote_names = 1;
            break;
        case 'k':
            args->block_size = 1024;
            break;
        case 3:
            args->block_size = (unsigned)strtoul(optarg, NULL, 10);
            if (args->block_size == 0) {
                fprintf(stderr, "Invalid block size: %s\n", optarg);
                exit(1);
            }
            break;
        case 4:
            args->dirs_first = 1;
            break;
        case 2:
            if (strcmp(optarg, "always") == 0)
                args->color_mode = COLOR_ALWAYS;
            else if (strcmp(optarg, "auto") == 0)
                args->color_mode = COLOR_AUTO;
            else if (strcmp(optarg, "never") == 0)
                args->color_mode = COLOR_NEVER;
            else {
                fprintf(stderr, "Invalid argument for --color: %s\n", optarg);
                exit(1);
            }
            break;
        case 1:
            printf("Usage: %s [-a] [-A] [-l] [-i] [-t] [-u] [-c] [-S] [-X] [-f] [-U] [-r] [-R] [-d] [-p] [-I PAT] [-B] [-L] [-H] [-Z] [-F] [-C] [-x] [-m] [-1] [-h] [-n] [-g] [-o] [-s] [-k] [-Q] [-V] [--color=WHEN] [--block-size=SIZE] [--group-directories-first] [--almost-all] [--ignore=PAT] [--quote-name] [--help] [--version] [path]\n", argv[0]);
            printf("Default is to display information about symbolic links. Use -L to follow them or -H for command line arguments only. Context display with -Z is supported only on systems with SELinux.\n");
            exit(0);
            break;
        case 'V':
            printf("vls %s\n", VLS_VERSION);
            exit(0);
            break;
        default:
            fprintf(stderr, "Usage: %s [-a] [-A] [-l] [-i] [-t] [-u] [-c] [-S] [-X] [-f] [-U] [-r] [-R] [-d] [-p] [-I PAT] [-B] [-L] [-H] [-Z] [-F] [-C] [-x] [-m] [-1] [-h] [-n] [-g] [-o] [-s] [-k] [-Q] [-V] [--color=WHEN] [--block-size=SIZE] [--group-directories-first] [--almost-all] [--ignore=PAT] [--quote-name] [--help] [--version] [path]\n", argv[0]);
            exit(1);
        }
    }

    if (args->block_size == 0) {
        args->block_size = getenv("POSIXLY_CORRECT") ? 512 : 1024;
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
