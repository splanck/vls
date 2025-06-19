#include <stdio.h>
#include "list.h"
#include "args.h"
#include "color.h"
#include <sys/stat.h>
#include <ctype.h>
#include <unistd.h>

static int hyperlink_enabled(HyperlinkMode mode) {
    return mode == HYPERLINK_ALWAYS || (mode == HYPERLINK_AUTO && isatty(STDOUT_FILENO));
}

static void hyperlink_start(const char *target, HyperlinkMode mode) {
    if (hyperlink_enabled(mode))
        printf("\033]8;;%s\033\\", target);
}

static void hyperlink_end(HyperlinkMode mode) {
    if (hyperlink_enabled(mode))
        printf("\033]8;;\033\\");
}

static void print_quoted(const char *s, QuotingStyle style, int hide_control, int show_controls, int literal_names) {
    if (literal_names) {
        fputs(s, stdout);
        return;
    }
    int quote = (style == QUOTE_C);
    int escape_nonprint = (style == QUOTE_C || style == QUOTE_ESCAPE);
    if (show_controls) {
        hide_control = 0;
        escape_nonprint = 0;
    }
    if (!quote && !escape_nonprint && !hide_control) {
        fputs(s, stdout);
        return;
    }
    if (quote)
        putchar('"');
    for (const char *p = s; *p; p++) {
        unsigned char c = (unsigned char)*p;
        if (quote && (c == '"' || c == '\\'))
            putchar('\\');
        if (!isprint(c)) {
            if (hide_control) {
                putchar('?');
            } else if (escape_nonprint) {
                char buf[5];
                snprintf(buf, sizeof(buf), "\\%03o", c);
                fputs(buf, stdout);
            } else {
                putchar(c);
            }
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
            hyperlink_start(path, args.hyperlink_mode);
            print_quoted(path, args.quoting_style, args.hide_control, args.show_controls, args.literal_names);
            hyperlink_end(args.hyperlink_mode);
            printf(":\n");
        }

        if (args.deref_cmdline) {
            struct stat st;
            if (stat(path, &st) == -1) {
                perror("stat");
                continue;
            }
            if (args.list_dirs_only || !S_ISDIR(st.st_mode)) {
                list_directory(path, args.color_mode, args.hyperlink_mode, args.show_hidden, args.almost_all,
                              args.long_format, args.show_inode, args.sort_time,
                              args.sort_atime, args.sort_ctime, args.sort_size, args.sort_extension,
                              args.sort_version, args.sort_word, args.unsorted, args.reverse, args.dirs_first, args.recursive,
                              args.indicator_style, args.human_readable,
                              args.human_si, args.numeric_ids, args.hide_owner, args.hide_group,
                              args.show_context, 1, 1, args.ignore_backups,
                                args.ignore_patterns, args.ignore_count,
                                args.hide_patterns, args.hide_count,
                                args.columns, args.across_columns, args.one_per_line, args.comma_separated,
                                args.show_blocks, args.quoting_style, args.time_word, args.time_style, args.block_size, args.hide_control, args.show_controls, args.literal_names);
                if (i < args.path_count - 1)
                    printf("\n");
                continue;
            }
        }

        list_directory(path, args.color_mode, args.hyperlink_mode, args.show_hidden, args.almost_all,
                      args.long_format, args.show_inode, args.sort_time,
                      args.sort_atime, args.sort_ctime, args.sort_size, args.sort_extension,
                      args.sort_version, args.sort_word, args.unsorted, args.reverse, args.dirs_first, args.recursive,
                      args.indicator_style, args.human_readable,
                      args.human_si, args.numeric_ids, args.hide_owner, args.hide_group,
                      args.show_context, args.follow_links, args.list_dirs_only, args.ignore_backups,
                        args.ignore_patterns, args.ignore_count,
                        args.hide_patterns, args.hide_count,
                        args.columns, args.across_columns, args.one_per_line, args.comma_separated,
                        args.show_blocks, args.quoting_style, args.time_word, args.time_style, args.block_size, args.hide_control, args.show_controls, args.literal_names);
        if (i < args.path_count - 1)
            printf("\n");
    }
    return 0;
}
