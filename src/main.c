#include <stdio.h>
#include "list.h"
#include "args.h"
#include "color.h"
#include <sys/stat.h>
#include <ctype.h>

static void print_quoted(const char *s, int quote, int escape_nonprint) {
    if (!quote && !escape_nonprint) {
        fputs(s, stdout);
        return;
    }
    if (quote)
        putchar('"');
    for (const char *p = s; *p; p++) {
        unsigned char c = (unsigned char)*p;
        if (quote && (c == '"' || c == '\\'))
            putchar('\\');
        if (escape_nonprint && !isprint(c)) {
            char buf[5];
            snprintf(buf, sizeof(buf), "\\%03o", c);
            fputs(buf, stdout);
        } else {
            putchar(c);
        }
    }
    if (quote)
        putchar('"');
}

int main(int argc, char *argv[]) {
    Args args;
    parse_args(argc, argv, &args);
    color_init();
    for (size_t i = 0; i < args.path_count; i++) {
        const char *path = args.paths[i];
        if (!args.recursive && args.path_count > 1 && !args.list_dirs_only) {
            print_quoted(path, args.quote_names, args.escape_nonprint);
            printf(":\n");
        }

        if (args.deref_cmdline) {
            struct stat st;
            if (stat(path, &st) == -1) {
                perror("stat");
                continue;
            }
            if (args.list_dirs_only || !S_ISDIR(st.st_mode)) {
                list_directory(path, args.color_mode, args.show_hidden, args.almost_all,
                              args.long_format, args.show_inode, args.sort_time,
                              args.sort_atime, args.sort_ctime, args.sort_size, args.sort_extension,
                              args.sort_version, args.unsorted, args.reverse, args.dirs_first, args.recursive,
                              args.classify, args.slash_dirs, args.human_readable,
                              args.numeric_ids, args.hide_owner, args.hide_group,
                              args.show_context, 1, 1, args.ignore_backups,
                                args.ignore_patterns, args.ignore_count, args.columns,
                                args.across_columns, args.one_per_line, args.comma_separated,
                                args.show_blocks, args.quote_names, args.escape_nonprint, args.block_size);
                if (i < args.path_count - 1)
                    printf("\n");
                continue;
            }
        }

        list_directory(path, args.color_mode, args.show_hidden, args.almost_all,
                      args.long_format, args.show_inode, args.sort_time,
                      args.sort_atime, args.sort_ctime, args.sort_size, args.sort_extension,
                      args.sort_version, args.unsorted, args.reverse, args.dirs_first, args.recursive,
                      args.classify, args.slash_dirs, args.human_readable,
                      args.numeric_ids, args.hide_owner, args.hide_group,
                      args.show_context, args.follow_links, args.list_dirs_only, args.ignore_backups,
                        args.ignore_patterns, args.ignore_count, args.columns,
                        args.across_columns, args.one_per_line, args.comma_separated,
                        args.show_blocks, args.quote_names, args.escape_nonprint, args.block_size);
        if (i < args.path_count - 1)
            printf("\n");
    }
    return 0;
}
