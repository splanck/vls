#include <stdio.h>
#include "list.h"
#include "args.h"
#include "color.h"

#define VLS_VERSION "0.1"

int main(int argc, char *argv[]) {
    Args args;
    parse_args(argc, argv, &args);
    color_init();

    printf("vls %s\n", VLS_VERSION);
    for (size_t i = 0; i < args.path_count; i++) {
        const char *path = args.paths[i];
        if (!args.recursive && args.path_count > 1 && !args.list_dirs_only)
            printf("%s:\n", path);
        list_directory(path, args.color_mode, args.show_hidden, args.almost_all,
                      args.long_format, args.show_inode, args.sort_time,
                      args.sort_atime, args.sort_size, args.sort_extension, args.unsorted, args.reverse, args.recursive,
                      args.classify, args.slash_dirs, args.human_readable,
                      args.numeric_ids, args.hide_owner, args.hide_group,
                      args.follow_links, args.list_dirs_only, args.ignore_backups,
                      args.columns, args.one_per_line);
        if (i < args.path_count - 1)
            printf("\n");
    }
    return 0;
}
