#include <stdio.h>
#include "list.h"
#include "args.h"

#define VLS_VERSION "0.1"

int main(int argc, char *argv[]) {
    Args args;
    parse_args(argc, argv, &args);

    printf("vls %s\n", VLS_VERSION);
    list_directory(args.path, args.use_color, args.show_hidden, args.long_format, args.show_inode, args.sort_time, args.sort_size, args.reverse, args.recursive, args.classify, args.human_readable);
    return 0;
}
