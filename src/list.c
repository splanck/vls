#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#if defined(__APPLE__) || defined(__NetBSD__) || defined(__FreeBSD__)
# include <sys/param.h>
# ifndef PATH_MAX
#  define PATH_MAX MAXPATHLEN
# endif
#endif
#ifndef PATH_MAX
# define PATH_MAX 4096
#endif
#include "list.h"
#include "color.h"

void list_directory(const char *path, int use_color, int show_hidden, int long_format) {
    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    char fullpath[PATH_MAX];
    while ((entry = readdir(dir)) != NULL) {
        if (!show_hidden && entry->d_name[0] == '.')
            continue;

        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);
        struct stat st;
        if (lstat(fullpath, &st) == -1) {
            perror("stat");
            continue;
        }

        const char *prefix = "";
        const char *suffix = "";
        if (use_color) {
            if (S_ISDIR(st.st_mode))
                prefix = color_dir();
            else if (S_ISLNK(st.st_mode))
                prefix = color_link();
            else if (st.st_mode & S_IXUSR)
                prefix = color_exec();
            suffix = color_reset();
        }

        if (long_format)
            printf("%s%10lld %s%s\n", prefix, (long long)st.st_size, entry->d_name, suffix);
        else
            printf("%s%s%s\n", prefix, entry->d_name, suffix);
    }

    closedir(dir);
}
