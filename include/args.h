#ifndef ARGS_H
#define ARGS_H

#include <stddef.h>

typedef enum {
    COLOR_NEVER,
    COLOR_ALWAYS,
    COLOR_AUTO
} ColorMode;

typedef enum {
    INDICATOR_NONE,
    INDICATOR_SLASH,
    INDICATOR_FILE_TYPE,
    INDICATOR_CLASSIFY
} IndicatorStyle;

typedef struct {
    const char **paths;
    size_t path_count;
    ColorMode color_mode;
    int show_hidden;
    int almost_all;
    int long_format;
    int show_inode;
    int sort_time;
    int sort_atime;
    int sort_ctime;
    int sort_size;
    int sort_extension;
    int sort_version;
    const char *sort_word;
    int unsorted;
    int reverse;
    int dirs_first;
    int recursive;
    int list_dirs_only;
    int follow_links;
    int deref_cmdline;
    int human_readable;
    int numeric_ids;
    int hide_owner;
    int hide_group;
    int show_context;
    IndicatorStyle indicator_style;
    int ignore_backups;
    const char **ignore_patterns;
    size_t ignore_count;
    const char **hide_patterns;
    size_t hide_count;
    int columns;
    int across_columns;
    int one_per_line;
    int comma_separated;
    int show_blocks;
    int quote_names;
    int escape_nonprint;
    const char *time_word;
    const char *time_style;
    unsigned block_size;
} Args;

void parse_args(int argc, char *argv[], Args *args);

#endif // ARGS_H
