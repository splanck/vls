#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
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

static int cmp_names(const void *a, const void *b) {
    const char *sa = *(const char **)a;
    const char *sb = *(const char **)b;
    return strcmp(sa, sb);
}

void list_directory(const char *path, int use_color, int show_hidden, int long_format, int reverse) {
    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    size_t count = 0, capacity = 32;
    char **names = malloc(capacity * sizeof(char *));
    if (!names) {
        perror("malloc");
        closedir(dir);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (!show_hidden && entry->d_name[0] == '.')
            continue;
        if (count == capacity) {
            capacity *= 2;
            char **tmp = realloc(names, capacity * sizeof(char *));
            if (!tmp) {
                perror("realloc");
                goto cleanup;
            }
            names = tmp;
        }
        names[count++] = strdup(entry->d_name);
    }

    qsort(names, count, sizeof(char *), cmp_names);

    char fullpath[PATH_MAX];
    for (size_t i = 0; i < count; i++) {
        size_t idx = reverse ? count - 1 - i : i;
        const char *name = names[idx];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, name);
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
            printf("%s%10lld %s%s\n", prefix, (long long)st.st_size, name, suffix);
        else
            printf("%s%s%s\n", prefix, name, suffix);
    }

cleanup:
    for (size_t i = 0; i < count; i++)
        free(names[i]);
    free(names);
    closedir(dir);
}
