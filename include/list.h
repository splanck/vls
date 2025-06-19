#ifndef LIST_H
#define LIST_H

#include "args.h"

void list_directory(const char *path, ColorMode color_mode, HyperlinkMode hyperlink_mode, int show_hidden, int almost_all, int long_format, int show_inode, int sort_time, int sort_atime, int sort_ctime, int sort_size, int sort_extension, int sort_version, const char *sort_word, int unsorted, int reverse, int dirs_first, int recursive, IndicatorStyle indicator_style, int human_readable, int numeric_ids, int hide_owner, int hide_group, int show_context, int follow_links, int list_dirs_only, int ignore_backups, const char **ignore_patterns, size_t ignore_count, const char **hide_patterns, size_t hide_count, int columns, int across_columns, int one_per_line, int comma_separated, int show_blocks, QuotingStyle quoting_style, const char *time_word, const char *time_style, unsigned block_size, int hide_control, int show_controls, int literal_names);

#endif // LIST_H
