#ifndef ARGS_H
#define ARGS_H

typedef struct {
    const char *path;
    int use_color;
    int show_hidden;
    int long_format;
    int reverse;
} Args;

void parse_args(int argc, char *argv[], Args *args);

#endif // ARGS_H
