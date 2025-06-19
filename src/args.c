#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "args.h"
#include <string.h>
#include <unistd.h>
#include "version.h"

void parse_args(int argc, char *argv[], Args *args) {
    args->color_mode = COLOR_AUTO;
    args->hyperlink_mode = HYPERLINK_AUTO;
    args->show_hidden = 0;
    args->almost_all = 0;
    args->long_format = 0;
    args->show_inode = 0;
    args->sort_time = 0;
    args->sort_atime = 0;
    args->sort_ctime = 0;
    args->sort_size = 0;
    args->sort_extension = 0;
    args->sort_version = 0;
    args->sort_word = NULL;
    args->unsorted = 0;
    args->reverse = 0;
    args->dirs_first = 0;
    args->recursive = 0;
    args->list_dirs_only = 0;
    args->indicator_style = INDICATOR_NONE;
    args->follow_links = 0;
    args->deref_cmdline = 0;
    args->human_readable = 0;
    args->human_si = 0;
    args->numeric_ids = 0;
    args->hide_owner = 0;
    args->hide_group = 0;
    args->show_context = 0;
    args->ignore_backups = 0;
    args->ignore_patterns = NULL;
    args->ignore_count = 0;
    args->hide_patterns = NULL;
    args->hide_count = 0;
    args->columns = isatty(STDOUT_FILENO);
    args->across_columns = 0;
    args->one_per_line = 0;
    args->comma_separated = 0;
    args->show_blocks = 0;
    args->quoting_style = QUOTE_LITERAL;
    args->hide_control = 0;
    args->show_controls = 0;
    args->literal_names = 0;
    args->time_word = NULL;
    args->time_style = "%b %e %H:%M";
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
        {"time-style", required_argument, 0, 5},
        {"full-time", no_argument, 0, 6},
        {"time", required_argument, 0, 10},
        {"file-type", no_argument, 0, 7},
        {"indicator-style", required_argument, 0, 11},
        {"hide", required_argument, 0, 8},
        {"sort", required_argument, 0, 9},
        {"quote-name", no_argument, 0, 'Q'},
        {"quoting-style", required_argument, 0, 12},
        {"literal", no_argument, 0, 'N'},
        {"hide-control-chars", no_argument, 0, 'q'},
        {"show-control-chars", no_argument, 0, 13},
        {"hyperlink", required_argument, 0, 14},
        {"si", no_argument, 0, 15},
        {"help", no_argument, 0, 1},
        {"version", no_argument, 0, 'V'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "AialtrucUfhXvRFpI:BhHLZdgonCx1msbQVNkq", long_options, NULL)) != -1) {
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
        case 'v':
            args->sort_version = 1;
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
            args->indicator_style = INDICATOR_SLASH;
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
            args->indicator_style = INDICATOR_CLASSIFY;
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
        case 'b':
            args->quoting_style = QUOTE_ESCAPE;
            break;
        case 'Q':
            args->quoting_style = QUOTE_C;
            break;
        case 'N':
            args->literal_names = 1;
            break;
        case 'q':
            args->hide_control = 1;
            break;
        case 13:
            args->show_controls = 1;
            args->hide_control = 0;
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
        case 5:
            args->time_style = optarg;
            break;
        case 6:
            args->time_style = "%F %T %z";
            break;
        case 10:
            args->time_word = optarg;
            if (strcmp(optarg, "mod") != 0 && strcmp(optarg, "access") != 0 &&
                strcmp(optarg, "use") != 0 && strcmp(optarg, "status") != 0) {
                fprintf(stderr, "Invalid time option: %s\n", optarg);
                exit(1);
            }
            break;
        case 7:
            args->indicator_style = INDICATOR_FILE_TYPE;
            break;
        case 11:
            if (strcmp(optarg, "none") == 0)
                args->indicator_style = INDICATOR_NONE;
            else if (strcmp(optarg, "slash") == 0)
                args->indicator_style = INDICATOR_SLASH;
            else if (strcmp(optarg, "file-type") == 0)
                args->indicator_style = INDICATOR_FILE_TYPE;
            else if (strcmp(optarg, "classify") == 0)
                args->indicator_style = INDICATOR_CLASSIFY;
            else {
                fprintf(stderr, "Invalid indicator style: %s\n", optarg);
                exit(1);
            }
            break;
        case 12:
            if (strcmp(optarg, "literal") == 0)
                args->quoting_style = QUOTE_LITERAL;
            else if (strcmp(optarg, "c") == 0)
                args->quoting_style = QUOTE_C;
            else if (strcmp(optarg, "escape") == 0)
                args->quoting_style = QUOTE_ESCAPE;
            else {
                fprintf(stderr, "Invalid quoting style: %s\n", optarg);
                exit(1);
            }
            break;
        case 14:
            if (strcmp(optarg, "always") == 0)
                args->hyperlink_mode = HYPERLINK_ALWAYS;
            else if (strcmp(optarg, "auto") == 0)
                args->hyperlink_mode = HYPERLINK_AUTO;
            else if (strcmp(optarg, "never") == 0)
                args->hyperlink_mode = HYPERLINK_NEVER;
            else {
                fprintf(stderr, "Invalid argument for --hyperlink: %s\n", optarg);
                exit(1);
            }
            break;
        case 15:
            args->human_si = 1;
            break;
        case 8:
            args->hide_patterns = realloc(args->hide_patterns,
                                          (args->hide_count + 1) * sizeof(char *));
            if (!args->hide_patterns) {
                perror("realloc");
                exit(1);
            }
            args->hide_patterns[args->hide_count++] = optarg;
            break;
        case 9:
            args->sort_word = optarg;
            args->sort_time = args->sort_atime = args->sort_ctime = 0;
            args->sort_size = args->sort_extension = args->sort_version = 0;
            args->unsorted = 0;
            if (strcmp(optarg, "none") == 0)
                args->unsorted = 1;
            else if (strcmp(optarg, "size") == 0)
                args->sort_size = 1;
            else if (strcmp(optarg, "time") == 0)
                args->sort_time = 1;
            else if (strcmp(optarg, "atime") == 0)
                args->sort_atime = 1;
            else if (strcmp(optarg, "ctime") == 0)
                args->sort_ctime = 1;
            else if (strcmp(optarg, "extension") == 0)
                args->sort_extension = 1;
            else if (strcmp(optarg, "version") == 0)
                args->sort_version = 1;
            else {
                fprintf(stderr, "Invalid sort option: %s\n", optarg);
                exit(1);
            }
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
            printf("Usage: %s [-a] [-A] [-l] [-i] [-t] [-u] [-c] [-S] [-X] [-v] [-f] [-U] [-r] [-R] [-d] [-p] [-I PAT] [-B] [-L] [-H] [-Z] [-F] [-C] [-x] [-m] [-1] [-h] [--si] [-n] [-g] [-o] [-s] [-k] [-b] [-Q] [-N] [-q] [-V] [--color=WHEN] [--hyperlink=WHEN] [--block-size=SIZE] [--group-directories-first] [--time-style=FMT] [--full-time] [--time=WORD] [--file-type] [--indicator-style=STYLE] [--almost-all] [--ignore=PAT] [--hide=PAT] [--sort=WORD] [--quoting-style=STYLE] [--quote-name] [--literal] [--hide-control-chars] [--show-control-chars] [--help] [--version] [path]\n", argv[0]);
            printf("Default is to display information about symbolic links. Use -L to follow them or -H for command line arguments only. Context display with -Z is supported only on systems with SELinux.\n");
            exit(0);
            break;
        case 'V':
            printf("vls %s\n", VLS_VERSION);
            exit(0);
            break;
        default:
            fprintf(stderr, "Usage: %s [-a] [-A] [-l] [-i] [-t] [-u] [-c] [-S] [-X] [-v] [-f] [-U] [-r] [-R] [-d] [-p] [-I PAT] [-B] [-L] [-H] [-Z] [-F] [-C] [-x] [-m] [-1] [-h] [--si] [-n] [-g] [-o] [-s] [-k] [-b] [-Q] [-N] [-q] [-V] [--color=WHEN] [--hyperlink=WHEN] [--block-size=SIZE] [--group-directories-first] [--time-style=FMT] [--full-time] [--time=WORD] [--file-type] [--indicator-style=STYLE] [--almost-all] [--ignore=PAT] [--hide=PAT] [--sort=WORD] [--quoting-style=STYLE] [--quote-name] [--literal] [--hide-control-chars] [--show-control-chars] [--help] [--version] [path]\n", argv[0]);
            exit(1);
        }
    }

    if (args->literal_names) {
        args->quoting_style = QUOTE_LITERAL;
        args->hide_control = 0;
        args->show_controls = 0;
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
