#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
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

static void human_size(off_t size, char *buf, size_t bufsz) {
    const char suffixes[] = {'B','K','M','G','T','P'};
    double s = (double)size;
    int i = 0;
    while (s >= 1024 && i < 5) {
        s /= 1024;
        i++;
    }
    if (i == 0)
        snprintf(buf, bufsz, "%lld%c", (long long)s, suffixes[i]);
    else
        snprintf(buf, bufsz, "%.1f%c", s, suffixes[i]);
}

static size_t num_digits(unsigned long long n) {
    size_t d = 1;
    while (n >= 10) {
        n /= 10;
        d++;
    }
    return d;
}

void list_directory(const char *path, int use_color, int show_hidden, int almost_all, int long_format, int show_inode, int sort_time, int sort_size, int reverse, int recursive, int classify, int human_readable, int follow_links, int list_dirs_only) {
    if (list_dirs_only) {
        struct stat st;
        int (*stat_fn)(const char *, struct stat *) = follow_links ? stat : lstat;
        if (stat_fn(path, &st) == -1) {
            perror("stat");
            return;
        }

        const char *prefix = "";
        const char *suffix = "";
        const char *indicator = "";
        if (use_color) {
            if (S_ISDIR(st.st_mode))
                prefix = color_dir();
            else if (S_ISLNK(st.st_mode))
                prefix = color_link();
            else if (st.st_mode & S_IXUSR)
                prefix = color_exec();
            suffix = color_reset();
        }
        if (classify) {
            if (S_ISDIR(st.st_mode))
                indicator = "/";
            else if (S_ISLNK(st.st_mode))
                indicator = "@";
            else if (st.st_mode & S_IXUSR)
                indicator = "*";
        }

        if (long_format) {
            char size_buf[16];
            if (human_readable)
                human_size(st.st_size, size_buf, sizeof(size_buf));
            else
                snprintf(size_buf, sizeof(size_buf), "%lld", (long long)st.st_size);

            struct passwd *pw = getpwuid(st.st_uid);
            char owner_buf[32];
            if (pw)
                snprintf(owner_buf, sizeof(owner_buf), "%s", pw->pw_name);
            else
                snprintf(owner_buf, sizeof(owner_buf), "%u", st.st_uid);

            struct group *gr = getgrgid(st.st_gid);
            char group_buf[32];
            if (gr)
                snprintf(group_buf, sizeof(group_buf), "%s", gr->gr_name);
            else
                snprintf(group_buf, sizeof(group_buf), "%u", st.st_gid);

            char perms[11];
            perms[0] = S_ISDIR(st.st_mode) ? 'd' :
                       S_ISLNK(st.st_mode) ? 'l' :
                       S_ISCHR(st.st_mode) ? 'c' :
                       S_ISBLK(st.st_mode) ? 'b' :
                       S_ISFIFO(st.st_mode) ? 'p' :
                       S_ISSOCK(st.st_mode) ? 's' : '-';
            perms[1] = (st.st_mode & S_IRUSR) ? 'r' : '-';
            perms[2] = (st.st_mode & S_IWUSR) ? 'w' : '-';
            perms[3] = (st.st_mode & S_IXUSR) ? 'x' : '-';
            perms[4] = (st.st_mode & S_IRGRP) ? 'r' : '-';
            perms[5] = (st.st_mode & S_IWGRP) ? 'w' : '-';
            perms[6] = (st.st_mode & S_IXGRP) ? 'x' : '-';
            perms[7] = (st.st_mode & S_IROTH) ? 'r' : '-';
            perms[8] = (st.st_mode & S_IWOTH) ? 'w' : '-';
            perms[9] = (st.st_mode & S_IXOTH) ? 'x' : '-';
            perms[10] = '\0';

            char time_buf[32];
            struct tm *tm = localtime(&st.st_mtime);
            strftime(time_buf, sizeof(time_buf), "%b %e %H:%M", tm);

            if (show_inode)
                printf("%10llu %s 1 %-*s %-*s %*s %s %s%s%s%s\n",
                       (unsigned long long)st.st_ino, perms,
                       (int)strlen(owner_buf), owner_buf,
                       (int)strlen(group_buf), group_buf,
                       (int)strlen(size_buf), size_buf,
                       time_buf, prefix, path, suffix, indicator);
            else
                printf("%s 1 %-*s %-*s %*s %s %s%s%s%s\n",
                       perms,
                       (int)strlen(owner_buf), owner_buf,
                       (int)strlen(group_buf), group_buf,
                       (int)strlen(size_buf), size_buf,
                       time_buf, prefix, path, suffix, indicator);
        } else {
            if (show_inode)
                printf("%10llu %s%s%s%s\n", (unsigned long long)st.st_ino, prefix, path, suffix, indicator);
            else
                printf("%s%s%s%s\n", prefix, path, suffix, indicator);
        }
        return;
    }

    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return;
    }

    if (recursive)
        printf("%s:\n", path);

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
        if (almost_all && (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0))
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
        int (*stat_fn)(const char *, struct stat *) = follow_links ? stat : lstat;
        if (stat_fn(fullpath, &entries[count].st) == -1) {
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

    size_t link_w = 0, owner_w = 0, group_w = 0, size_w = 0;
    if (long_format) {
        for (size_t i = 0; i < count; i++) {
            const Entry *ent = &entries[i];
            if (num_digits(ent->st.st_nlink) > link_w)
                link_w = num_digits(ent->st.st_nlink);

            struct passwd *pw = getpwuid(ent->st.st_uid);
            size_t len = pw ? strlen(pw->pw_name) : num_digits(ent->st.st_uid);
            if (len > owner_w)
                owner_w = len;

            struct group *gr = getgrgid(ent->st.st_gid);
            len = gr ? strlen(gr->gr_name) : num_digits(ent->st.st_gid);
            if (len > group_w)
                group_w = len;

            char sz[16];
            if (human_readable)
                human_size(ent->st.st_size, sz, sizeof(sz));
            else
                snprintf(sz, sizeof(sz), "%lld", (long long)ent->st.st_size);
            len = strlen(sz);
            if (len > size_w)
                size_w = len;
        }
    }

    for (size_t i = 0; i < count; i++) {
        size_t idx = reverse ? count - 1 - i : i;
        const Entry *ent = &entries[idx];

        const char *prefix = "";
        const char *suffix = "";
        const char *indicator = "";
        if (use_color) {
            if (S_ISDIR(ent->st.st_mode))
                prefix = color_dir();
            else if (S_ISLNK(ent->st.st_mode))
                prefix = color_link();
            else if (ent->st.st_mode & S_IXUSR)
                prefix = color_exec();
            suffix = color_reset();
        }
        if (classify) {
            if (S_ISDIR(ent->st.st_mode))
                indicator = "/";
            else if (S_ISLNK(ent->st.st_mode))
                indicator = "@";
            else if (ent->st.st_mode & S_IXUSR)
                indicator = "*";
        }

        if (long_format) {
            char size_buf[16];
            if (human_readable)
                human_size(ent->st.st_size, size_buf, sizeof(size_buf));
            else
                snprintf(size_buf, sizeof(size_buf), "%lld", (long long)ent->st.st_size);

            struct passwd *pw = getpwuid(ent->st.st_uid);
            char owner_buf[32];
            if (pw)
                snprintf(owner_buf, sizeof(owner_buf), "%s", pw->pw_name);
            else
                snprintf(owner_buf, sizeof(owner_buf), "%u", ent->st.st_uid);

            struct group *gr = getgrgid(ent->st.st_gid);
            char group_buf[32];
            if (gr)
                snprintf(group_buf, sizeof(group_buf), "%s", gr->gr_name);
            else
                snprintf(group_buf, sizeof(group_buf), "%u", ent->st.st_gid);

            char perms[11];
            perms[0] = S_ISDIR(ent->st.st_mode) ? 'd' :
                       S_ISLNK(ent->st.st_mode) ? 'l' :
                       S_ISCHR(ent->st.st_mode) ? 'c' :
                       S_ISBLK(ent->st.st_mode) ? 'b' :
                       S_ISFIFO(ent->st.st_mode) ? 'p' :
                       S_ISSOCK(ent->st.st_mode) ? 's' : '-';
            perms[1] = (ent->st.st_mode & S_IRUSR) ? 'r' : '-';
            perms[2] = (ent->st.st_mode & S_IWUSR) ? 'w' : '-';
            perms[3] = (ent->st.st_mode & S_IXUSR) ? 'x' : '-';
            perms[4] = (ent->st.st_mode & S_IRGRP) ? 'r' : '-';
            perms[5] = (ent->st.st_mode & S_IWGRP) ? 'w' : '-';
            perms[6] = (ent->st.st_mode & S_IXGRP) ? 'x' : '-';
            perms[7] = (ent->st.st_mode & S_IROTH) ? 'r' : '-';
            perms[8] = (ent->st.st_mode & S_IWOTH) ? 'w' : '-';
            perms[9] = (ent->st.st_mode & S_IXOTH) ? 'x' : '-';
            perms[10] = '\0';

            char time_buf[32];
            struct tm *tm = localtime(&ent->st.st_mtime);
            strftime(time_buf, sizeof(time_buf), "%b %e %H:%M", tm);

            if (show_inode)
                printf("%10llu %s %*lu %-*s %-*s %*s %s %s%s%s%s\n",
                       (unsigned long long)ent->st.st_ino, perms,
                       (int)link_w, (unsigned long)ent->st.st_nlink,
                       (int)owner_w, owner_buf,
                       (int)group_w, group_buf,
                       (int)size_w, size_buf,
                       time_buf, prefix, ent->name, suffix, indicator);
            else
                printf("%s %*lu %-*s %-*s %*s %s %s%s%s%s\n",
                       perms,
                       (int)link_w, (unsigned long)ent->st.st_nlink,
                       (int)owner_w, owner_buf,
                       (int)group_w, group_buf,
                       (int)size_w, size_buf,
                       time_buf, prefix, ent->name, suffix, indicator);
        } else {
            if (show_inode)
                printf("%10llu %s%s%s%s\n", (unsigned long long)ent->st.st_ino, prefix, ent->name, suffix, indicator);
            else
                printf("%s%s%s%s\n", prefix, ent->name, suffix, indicator);
        }
    }

    if (recursive) {
        for (size_t i = 0; i < count; i++) {
            size_t idx = reverse ? count - 1 - i : i;
            const Entry *ent = &entries[idx];
            if (!S_ISDIR(ent->st.st_mode) || S_ISLNK(ent->st.st_mode))
                continue;
            if (strcmp(ent->name, ".") == 0 || strcmp(ent->name, "..") == 0)
                continue;
            char fullpath[PATH_MAX];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", path, ent->name);
            printf("\n");
            list_directory(fullpath, use_color, show_hidden, almost_all, long_format, show_inode, sort_time, sort_size, reverse, recursive, classify, human_readable, follow_links, list_dirs_only);
        }
    }

cleanup:
    for (size_t i = 0; i < count; i++)
        free(entries[i].name);
    free(entries);
    closedir(dir);
}
