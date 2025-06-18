#ifndef ARGS_H
#define ARGS_H

#include <stddef.h>

typedef struct {
    const char **paths;
    size_t path_count;
    int use_color;
    int show_hidden;
    int almost_all;
    int long_format;
    int show_inode;
    int sort_time;
    int sort_atime;
    int sort_size;
    int reverse;
    int recursive;
    int list_dirs_only;
    int follow_links;
    int human_readable;
    int numeric_ids;
    int hide_owner;
    int hide_group;
    int classify;
} Args;

void parse_args(int argc, char *argv[], Args *args);

#endif // ARGS_H
