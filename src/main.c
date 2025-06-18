#include <stdio.h>
#include "list.h"
#include "args.h"

#define VLS_VERSION "0.1"

int main(int argc, char *argv[]) {
    Args args;
    parse_args(argc, argv, &args);

    printf("vls %s\n", VLS_VERSION);
    for (size_t i = 0; i < args.path_count; i++) {
        const char *path = args.paths[i];
        if (!args.recursive && args.path_count > 1 && !args.list_dirs_only)
            printf("%s:\n", path);
        list_directory(path, args.use_color, args.show_hidden, args.almost_all,
                      args.long_format, args.show_inode, args.sort_time,
                      args.sort_size, args.reverse, args.recursive,
                      args.classify, args.human_readable,
                      args.follow_links, args.list_dirs_only);
        if (i < args.path_count - 1)
            printf("\n");
    }
    return 0;
}
