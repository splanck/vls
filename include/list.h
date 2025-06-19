#ifndef LIST_H
#define LIST_H

#include "args.h"

void list_directory(const char *path, ColorMode color_mode, int show_hidden, int almost_all, int long_format, int show_inode, int sort_time, int sort_atime, int sort_size, int unsorted, int reverse, int recursive, int classify, int slash_dirs, int human_readable, int numeric_ids, int hide_owner, int hide_group, int follow_links, int list_dirs_only, int ignore_backups, int columns, int one_per_line);

#endif // LIST_H
