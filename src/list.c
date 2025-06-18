#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <linux/limits.h>
#include "list.h"
#include "color.h"

void list_directory(const char *path, int use_color) {
    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    char fullpath[PATH_MAX];
    while ((entry = readdir(dir)) != NULL) {
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
        printf("%s%s%s\n", prefix, entry->d_name, suffix);
    }

    closedir(dir);
}
