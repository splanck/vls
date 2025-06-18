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

typedef struct {
    char *name;
    struct stat st;
} Entry;

static int cmp_names(const void *a, const void *b) {
    const Entry *ea = a;
    const Entry *eb = b;
    return strcmp(ea->name, eb->name);
}

static int cmp_mtime(const void *a, const void *b) {
    const Entry *ea = a;
    const Entry *eb = b;
    if (ea->st.st_mtime == eb->st.st_mtime)
        return 0;
    return (ea->st.st_mtime > eb->st.st_mtime) ? -1 : 1;
}

static int cmp_size(const void *a, const void *b) {
    const Entry *ea = a;
    const Entry *eb = b;
    if (ea->st.st_size == eb->st.st_size)
        return 0;
    return (ea->st.st_size > eb->st.st_size) ? -1 : 1;
}

void list_directory(const char *path, int use_color, int show_hidden, int long_format, int sort_time, int sort_size, int reverse) {
    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    size_t count = 0, capacity = 32;
    Entry *entries = malloc(capacity * sizeof(Entry));
    if (!entries) {
        perror("malloc");
        closedir(dir);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (!show_hidden && entry->d_name[0] == '.')
            continue;
        if (count == capacity) {
            capacity *= 2;
            Entry *tmp = realloc(entries, capacity * sizeof(Entry));
            if (!tmp) {
                perror("realloc");
                goto cleanup;
            }
            entries = tmp;
        }
        entries[count].name = strdup(entry->d_name);
        if (!entries[count].name) {
            perror("strdup");
            goto cleanup;
        }
        char fullpath[PATH_MAX];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entries[count].name);
        if (lstat(fullpath, &entries[count].st) == -1) {
            perror("stat");
            free(entries[count].name);
            continue;
        }
        count++;
    }

    int (*cmp)(const void *, const void *) = cmp_names;
    if (sort_size)
        cmp = cmp_size;
    else if (sort_time)
        cmp = cmp_mtime;
    qsort(entries, count, sizeof(Entry), cmp);

    for (size_t i = 0; i < count; i++) {
        size_t idx = reverse ? count - 1 - i : i;
        const Entry *ent = &entries[idx];

        const char *prefix = "";
        const char *suffix = "";
        if (use_color) {
            if (S_ISDIR(ent->st.st_mode))
                prefix = color_dir();
            else if (S_ISLNK(ent->st.st_mode))
                prefix = color_link();
            else if (ent->st.st_mode & S_IXUSR)
                prefix = color_exec();
            suffix = color_reset();
        }

        if (long_format)
            printf("%s%10lld %s%s\n", prefix, (long long)ent->st.st_size, ent->name, suffix);
        else
            printf("%s%s%s\n", prefix, ent->name, suffix);
    }

cleanup:
    for (size_t i = 0; i < count; i++)
        free(entries[i].name);
    free(entries);
    closedir(dir);
}
