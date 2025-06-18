#ifndef ARGS_H
#define ARGS_H

typedef struct {
    const char *path;
    int use_color;
    int show_hidden;
    int long_format;
    int show_inode;
    int sort_time;
    int sort_size;
    int reverse;
    int recursive;
    int human_readable;
    int classify;
} Args;

void parse_args(int argc, char *argv[], Args *args);

#endif // ARGS_H
