#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <fnmatch.h>
#include <stdbool.h>
#if __has_include(<selinux/selinux.h>)
# include <selinux/selinux.h>
# define HAVE_SELINUX 1
#else
# define HAVE_SELINUX 0
#endif
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

static int cmp_atime(const void *a, const void *b) {
    const Entry *ea = a;
    const Entry *eb = b;
    if (ea->st.st_atime == eb->st.st_atime)
        return 0;
    return (ea->st.st_atime > eb->st.st_atime) ? -1 : 1;
}

static int cmp_ctime(const void *a, const void *b) {
    const Entry *ea = a;
    const Entry *eb = b;
    if (ea->st.st_ctime == eb->st.st_ctime)
        return 0;
    return (ea->st.st_ctime > eb->st.st_ctime) ? -1 : 1;
}

static int cmp_size(const void *a, const void *b) {
    const Entry *ea = a;
    const Entry *eb = b;
    if (ea->st.st_size == eb->st.st_size)
        return 0;
    return (ea->st.st_size > eb->st.st_size) ? -1 : 1;
}

static int cmp_extension(const void *a, const void *b) {
    const Entry *ea = a;
    const Entry *eb = b;
    const char *ea_ext = strrchr(ea->name, '.');
    const char *eb_ext = strrchr(eb->name, '.');
    ea_ext = ea_ext ? ea_ext + 1 : ea->name;
    eb_ext = eb_ext ? eb_ext + 1 : eb->name;
    int cmp = strcasecmp(ea_ext, eb_ext);
    if (cmp == 0)
        return strcasecmp(ea->name, eb->name);
    return cmp;
}

static int cmp_version(const void *a, const void *b) {
    const Entry *ea = a;
    const Entry *eb = b;
#if defined(__GLIBC__) || defined(__GNU_LIBRARY__) || defined(__linux__)
    return strverscmp(ea->name, eb->name);
#else
    const char *sa = ea->name;
    const char *sb = eb->name;
    while (*sa && *sb) {
        if (isdigit((unsigned char)*sa) && isdigit((unsigned char)*sb)) {
            char *ea_end; char *eb_end;
            unsigned long na = strtoul(sa, &ea_end, 10);
            unsigned long nb = strtoul(sb, &eb_end, 10);
            if (na != nb)
                return (na > nb) ? 1 : -1;
            sa = ea_end;
            sb = eb_end;
        } else {
            if (*sa != *sb)
                return (unsigned char)*sa - (unsigned char)*sb;
            sa++; sb++;
        }
    }
    if (*sa) return 1;
    if (*sb) return -1;
    return 0;
#endif
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

static size_t quoted_len(const char *s, int escape_nonprint, int hide_control) {
    size_t len = 2; /* surrounding quotes */
    for (const char *p = s; *p; p++) {
        unsigned char c = (unsigned char)*p;
        if (c == '"' || c == '\\')
            len++; /* for escape */
        if (!isprint(c)) {
            if (hide_control)
                len += 1;
            else if (escape_nonprint)
                len += 3; /* octal */
            else
                len += 1;
        } else {
            len++;
        }
    }
    return len;
}

static size_t escaped_len(const char *s, int hide_control) {
    size_t len = 0;
    for (const char *p = s; *p; p++) {
        unsigned char c = (unsigned char)*p;
        if (!isprint(c)) {
            if (hide_control)
                len += 1;
            else
                len += 4; /* backslash + 3 octal digits */
        } else {
            len++;
        }
    }
    return len;
}

static void print_quoted(const char *s, QuotingStyle style, int hide_control) {
    int quote = (style == QUOTE_C);
    int escape_nonprint = (style == QUOTE_C || style == QUOTE_ESCAPE);
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

void list_directory(const char *path, ColorMode color_mode, int show_hidden, int almost_all, int long_format, int show_inode, int sort_time, int sort_atime, int sort_ctime, int sort_size, int sort_extension, int sort_version, const char *sort_word, int unsorted, int reverse, int dirs_first, int recursive, IndicatorStyle indicator_style, int human_readable, int numeric_ids, int hide_owner, int hide_group, int show_context, int follow_links, int list_dirs_only, int ignore_backups, const char **ignore_patterns, size_t ignore_count, const char **hide_patterns, size_t hide_count, int columns, int across_columns, int one_per_line, int comma_separated, int show_blocks, QuotingStyle quoting_style, const char *time_word, const char *time_style, unsigned block_size, int hide_control) {
    int use_color = 0;
    if (color_mode == COLOR_ALWAYS)
        use_color = 1;
    else if (color_mode == COLOR_AUTO)
        use_color = isatty(STDOUT_FILENO);
    int quote_names = (quoting_style == QUOTE_C);
    int escape_nonprint = (quoting_style == QUOTE_C || quoting_style == QUOTE_ESCAPE);
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
        switch (indicator_style) {
        case INDICATOR_CLASSIFY:
            if (S_ISDIR(st.st_mode))
                indicator = "/";
            else if (S_ISLNK(st.st_mode))
                indicator = "@";
            else if (st.st_mode & S_IXUSR)
                indicator = "*";
            break;
        case INDICATOR_FILE_TYPE:
            if (S_ISDIR(st.st_mode))
                indicator = "/";
            else if (S_ISLNK(st.st_mode))
                indicator = "@";
            break;
        case INDICATOR_SLASH:
            if (S_ISDIR(st.st_mode))
                indicator = "/";
            break;
        default:
            break;
        }

        unsigned long single_blocks = (unsigned long)((st.st_blocks * 512 + block_size - 1) / block_size);
        size_t single_w = num_digits(single_blocks);

        if (long_format) {
            char size_buf[16];
            if (human_readable)
                human_size(st.st_size, size_buf, sizeof(size_buf));
            else
                snprintf(size_buf, sizeof(size_buf), "%lld", (long long)st.st_size);

            struct passwd *pw = numeric_ids ? NULL : getpwuid(st.st_uid);
            char owner_buf[32];
            if (pw)
                snprintf(owner_buf, sizeof(owner_buf), "%s", pw->pw_name);
            else
                snprintf(owner_buf, sizeof(owner_buf), "%u", st.st_uid);

            struct group *gr = numeric_ids ? NULL : getgrgid(st.st_gid);
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
            const time_t *tptr = &st.st_mtime;
            if (time_word) {
                if (strcmp(time_word, "access") == 0 || strcmp(time_word, "use") == 0)
                    tptr = &st.st_atime;
                else if (strcmp(time_word, "status") == 0)
                    tptr = &st.st_ctime;
            } else {
                if (sort_atime)
                    tptr = &st.st_atime;
                else if (sort_ctime)
                    tptr = &st.st_ctime;
            }
            struct tm *tm = localtime(tptr);
            strftime(time_buf, sizeof(time_buf), time_style, tm);

            if (show_blocks)
                printf("%*lu ", (int)single_w, single_blocks);
            if (show_inode)
                printf("%10llu ", (unsigned long long)st.st_ino);
            printf("%s 1 ", perms);
            if (!hide_owner)
                printf("%-*s ", (int)strlen(owner_buf), owner_buf);
            if (!hide_group)
                printf("%-*s ", (int)strlen(group_buf), group_buf);
            printf("%*s %s", (int)strlen(size_buf), size_buf, time_buf);
            if (show_context) {
#if HAVE_SELINUX
                char *ctx = NULL;
                if (lgetfilecon(path, &ctx) >= 0) {
                    printf(" %s", ctx);
                    freecon(ctx);
                } else {
                    printf(" -");
                }
#else
                printf(" -");
#endif
            }
            printf(" %s", prefix);
            print_quoted(path, quoting_style, hide_control);
            printf("%s%s\n", suffix, indicator);
        } else {
            if (show_blocks)
                printf("%*lu ", (int)single_w, single_blocks);
            if (show_inode) {
                printf("%10llu %s", (unsigned long long)st.st_ino, prefix);
                print_quoted(path, quoting_style, hide_control);
                printf("%s%s\n", suffix, indicator);
            }
            else {
                fputs(prefix, stdout);
                print_quoted(path, quoting_style, hide_control);
                printf("%s%s\n", suffix, indicator);
            }
        }
        return;
    }

    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return;
    }

    if (recursive) {
        print_quoted(path, quoting_style, hide_control);
        printf(":\n");
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
        if (almost_all && (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0))
            continue;
        if (hide_patterns && !show_hidden && !almost_all) {
            int hide = 0;
            for (size_t i = 0; i < hide_count && !hide; i++)
                if (fnmatch(hide_patterns[i], entry->d_name, 0) == 0)
                    hide = 1;
            if (hide)
                continue;
        }
        if (ignore_backups) {
            size_t len = strlen(entry->d_name);
            if (len > 0 && entry->d_name[len - 1] == '~')
                continue;
        }
        if (ignore_patterns) {
            int matched = 0;
            for (size_t i = 0; i < ignore_count && !matched; i++)
                if (fnmatch(ignore_patterns[i], entry->d_name, 0) == 0)
                    matched = 1;
            if (matched)
                continue;
        }
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

    if (!unsorted) {
        int (*cmp)(const void *, const void *) = cmp_names;
        if (sort_word) {
            if (strcmp(sort_word, "size") == 0)
                cmp = cmp_size;
            else if (strcmp(sort_word, "time") == 0)
                cmp = cmp_mtime;
            else if (strcmp(sort_word, "atime") == 0)
                cmp = cmp_atime;
            else if (strcmp(sort_word, "ctime") == 0)
                cmp = cmp_ctime;
            else if (strcmp(sort_word, "extension") == 0)
                cmp = cmp_extension;
            else if (strcmp(sort_word, "version") == 0)
                cmp = cmp_version;
        } else {
            if (sort_size)
                cmp = cmp_size;
            else if (sort_time)
                cmp = cmp_mtime;
            else if (sort_atime)
                cmp = cmp_atime;
            else if (sort_ctime)
                cmp = cmp_ctime;
            else if (sort_extension)
                cmp = cmp_extension;
            else if (sort_version)
                cmp = cmp_version;
        }
        qsort(entries, count, sizeof(Entry), cmp);
    }

    if (dirs_first && count > 1) {
        Entry *tmp = malloc(count * sizeof(Entry));
        if (!tmp) {
            perror("malloc");
            goto cleanup;
        }
        size_t di = 0;
        size_t fi = 0;
        for (size_t i = 0; i < count; i++)
            if (S_ISDIR(entries[i].st.st_mode))
                fi++;
        di = 0;
        size_t dir_total = fi;
        fi = dir_total;
        for (size_t i = 0; i < count; i++) {
            if (S_ISDIR(entries[i].st.st_mode))
                tmp[di++] = entries[i];
            else
                tmp[fi++] = entries[i];
        }
        memcpy(entries, tmp, count * sizeof(Entry));
        free(tmp);
    }

    size_t link_w = 0, owner_w = 0, group_w = 0, size_w = 0, block_w = 0;
    unsigned long total_blocks = 0;
    size_t max_len = 0;
    for (size_t i = 0; i < count; i++) {
        const Entry *ent = &entries[i];
        unsigned long blk = (unsigned long)((ent->st.st_blocks * 512 + block_size - 1) / block_size);
        if (long_format || show_blocks)
            total_blocks += blk;
        if (show_blocks) {
            size_t d = num_digits(blk);
            if (d > block_w)
                block_w = d;
        }

        if (long_format) {
            if (num_digits(ent->st.st_nlink) > link_w)
                link_w = num_digits(ent->st.st_nlink);

            if (!hide_owner) {
                struct passwd *pw = numeric_ids ? NULL : getpwuid(ent->st.st_uid);
                size_t len = pw ? strlen(pw->pw_name) : num_digits(ent->st.st_uid);
                if (len > owner_w)
                    owner_w = len;
            }

            if (!hide_group) {
                struct group *gr = numeric_ids ? NULL : getgrgid(ent->st.st_gid);
                size_t len = gr ? strlen(gr->gr_name) : num_digits(ent->st.st_gid);
                if (len > group_w)
                    group_w = len;
            }

            char sz[16];
            if (human_readable)
                human_size(ent->st.st_size, sz, sizeof(sz));
            else
                snprintf(sz, sizeof(sz), "%lld", (long long)ent->st.st_size);
            size_t len_sz = strlen(sz);
            if (len_sz > size_w)
                size_w = len_sz;
        }

        size_t name_len = quote_names ? quoted_len(ent->name, escape_nonprint, hide_control) :
                            (escape_nonprint ? escaped_len(ent->name, hide_control) : strlen(ent->name));
        if (show_inode)
            name_len += num_digits(ent->st.st_ino) + 1;
        switch (indicator_style) {
        case INDICATOR_CLASSIFY:
            if (S_ISDIR(ent->st.st_mode) || (ent->st.st_mode & S_IXUSR) || S_ISLNK(ent->st.st_mode))
                name_len += 1;
            break;
        case INDICATOR_FILE_TYPE:
            if (S_ISDIR(ent->st.st_mode) || S_ISLNK(ent->st.st_mode))
                name_len += 1;
            break;
        case INDICATOR_SLASH:
            if (S_ISDIR(ent->st.st_mode))
                name_len += 1;
            break;
        default:
            break;
        }
        if (name_len > max_len)
            max_len = name_len;
    }

    if (show_blocks)
        max_len += block_w + 1;

    if (long_format || show_blocks)
        printf("total %lu\n", total_blocks);

    if (comma_separated && !long_format) {
        int term_width = 80;
        struct winsize ws;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
            term_width = ws.ws_col;
        size_t line_len = 0;
        for (size_t i = 0; i < count; i++) {
            size_t idx = reverse ? count - 1 - i : i;
            const Entry *ent = &entries[idx];
            unsigned long blk = (unsigned long)((ent->st.st_blocks * 512 + block_size - 1) / block_size);

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
            switch (indicator_style) {
            case INDICATOR_CLASSIFY:
                if (S_ISDIR(ent->st.st_mode))
                    indicator = "/";
                else if (S_ISLNK(ent->st.st_mode))
                    indicator = "@";
                else if (ent->st.st_mode & S_IXUSR)
                    indicator = "*";
                break;
            case INDICATOR_FILE_TYPE:
                if (S_ISDIR(ent->st.st_mode))
                    indicator = "/";
                else if (S_ISLNK(ent->st.st_mode))
                    indicator = "@";
                break;
            case INDICATOR_SLASH:
                if (S_ISDIR(ent->st.st_mode))
                    indicator = "/";
                break;
            default:
                break;
            }

            char block_buf[32] = "";
            if (show_blocks)
                snprintf(block_buf, sizeof(block_buf), "%*lu ", (int)block_w, blk);
            char inode_buf[32] = "";
            if (show_inode)
                snprintf(inode_buf, sizeof(inode_buf), "%10llu ", (unsigned long long)ent->st.st_ino);

            size_t len = strlen(block_buf) + strlen(inode_buf) +
                         (quote_names ? quoted_len(ent->name, escape_nonprint, hide_control) :
                          (escape_nonprint ? escaped_len(ent->name, hide_control) : strlen(ent->name))) +
                         strlen(indicator);
            if (line_len && line_len + len > (size_t)term_width) {
                putchar('\n');
                line_len = 0;
            }
            printf("%s%s%s", block_buf, inode_buf, prefix);
            print_quoted(ent->name, quoting_style, hide_control);
            printf("%s%s", suffix, indicator);
            line_len += len;
            if (i < count - 1) {
                if (line_len + 2 > (size_t)term_width) {
                    putchar('\n');
                    line_len = 0;
                } else {
                    printf(", ");
                    line_len += 2;
                }
            } else {
                putchar('\n');
            }
        }
    } else if (!long_format && columns && !one_per_line) {
        int term_width = 80;
        struct winsize ws;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
            term_width = ws.ws_col;
        size_t col_width = max_len + 2;
        size_t cols = term_width / (int)col_width;
        if (cols == 0)
            cols = 1;
        if (cols > count)
            cols = count;
        size_t rows = (count + cols - 1) / cols;

        if (across_columns) {
            for (size_t i = 0; i < count; i++) {
                size_t idx = reverse ? count - 1 - i : i;
                const Entry *ent = &entries[idx];
                unsigned long blk = (unsigned long)((ent->st.st_blocks * 512 + block_size - 1) / block_size);

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
            switch (indicator_style) {
            case INDICATOR_CLASSIFY:
                if (S_ISDIR(ent->st.st_mode))
                    indicator = "/";
                else if (S_ISLNK(ent->st.st_mode))
                    indicator = "@";
                else if (ent->st.st_mode & S_IXUSR)
                    indicator = "*";
                break;
            case INDICATOR_FILE_TYPE:
                if (S_ISDIR(ent->st.st_mode))
                    indicator = "/";
                else if (S_ISLNK(ent->st.st_mode))
                    indicator = "@";
                break;
            case INDICATOR_SLASH:
                if (S_ISDIR(ent->st.st_mode))
                    indicator = "/";
                break;
            default:
                break;
            }

            char block_buf[32] = "";
            if (show_blocks)
                snprintf(block_buf, sizeof(block_buf), "%*lu ", (int)block_w, blk);
            char inode_buf[32] = "";
            if (show_inode)
                snprintf(inode_buf, sizeof(inode_buf), "%10llu ", (unsigned long long)ent->st.st_ino);
            printf("%s%s%s", block_buf, inode_buf, prefix);
            print_quoted(ent->name, quoting_style, hide_control);
            printf("%s%s", suffix, indicator);

            size_t len = (quote_names ? quoted_len(ent->name, escape_nonprint, hide_control) :
                           (escape_nonprint ? escaped_len(ent->name, hide_control) : strlen(ent->name))) +
                         strlen(indicator) + strlen(inode_buf) + strlen(block_buf);
            if ((i % cols == cols - 1) || i == count - 1) {
                putchar('\n');
            } else {
                for (size_t sp = len; sp < col_width; sp++)
                    putchar(' ');
            }
        }
        } else {
            for (size_t r = 0; r < rows; r++) {
                for (size_t c = 0; c < cols; c++) {
                    size_t i = c * rows + r;
                    if (i >= count)
                        continue;
                    size_t idx = reverse ? count - 1 - i : i;
                    const Entry *ent = &entries[idx];
                    unsigned long blk = (unsigned long)((ent->st.st_blocks * 512 + block_size - 1) / block_size);

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
                    switch (indicator_style) {
                    case INDICATOR_CLASSIFY:
                        if (S_ISDIR(ent->st.st_mode))
                            indicator = "/";
                        else if (S_ISLNK(ent->st.st_mode))
                            indicator = "@";
                        else if (ent->st.st_mode & S_IXUSR)
                            indicator = "*";
                        break;
                    case INDICATOR_FILE_TYPE:
                        if (S_ISDIR(ent->st.st_mode))
                            indicator = "/";
                        else if (S_ISLNK(ent->st.st_mode))
                            indicator = "@";
                        break;
                    case INDICATOR_SLASH:
                        if (S_ISDIR(ent->st.st_mode))
                            indicator = "/";
                        break;
                    default:
                        break;
                    }

                    char block_buf[32] = "";
                    if (show_blocks)
                        snprintf(block_buf, sizeof(block_buf), "%*lu ", (int)block_w, blk);
                    char inode_buf[32] = "";
                    if (show_inode)
                        snprintf(inode_buf, sizeof(inode_buf), "%10llu ", (unsigned long long)ent->st.st_ino);
                    printf("%s%s%s", block_buf, inode_buf, prefix);
                    print_quoted(ent->name, quoting_style, hide_control);
                    printf("%s%s", suffix, indicator);

                    size_t len = (quote_names ? quoted_len(ent->name, escape_nonprint, hide_control) :
                                   (escape_nonprint ? escaped_len(ent->name, hide_control) : strlen(ent->name))) +
                                 strlen(indicator) + strlen(inode_buf) + strlen(block_buf);
                    if (c == cols - 1 || i + rows >= count) {
                        putchar('\n');
                    } else {
                        for (size_t sp = len; sp < col_width; sp++)
                            putchar(' ');
                    }
                }
            }
        }
    } else {
    for (size_t i = 0; i < count; i++) {
        size_t idx = reverse ? count - 1 - i : i;
        const Entry *ent = &entries[idx];
        unsigned long blk = (unsigned long)((ent->st.st_blocks * 512 + block_size - 1) / block_size);

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
        switch (indicator_style) {
        case INDICATOR_CLASSIFY:
            if (S_ISDIR(ent->st.st_mode))
                indicator = "/";
            else if (S_ISLNK(ent->st.st_mode))
                indicator = "@";
            else if (ent->st.st_mode & S_IXUSR)
                indicator = "*";
            break;
        case INDICATOR_FILE_TYPE:
            if (S_ISDIR(ent->st.st_mode))
                indicator = "/";
            else if (S_ISLNK(ent->st.st_mode))
                indicator = "@";
            break;
        case INDICATOR_SLASH:
            if (S_ISDIR(ent->st.st_mode))
                indicator = "/";
            break;
        default:
            break;
        }

        if (long_format || one_per_line || !columns) {
            char size_buf[16];
            if (human_readable)
                human_size(ent->st.st_size, size_buf, sizeof(size_buf));
            else
                snprintf(size_buf, sizeof(size_buf), "%lld", (long long)ent->st.st_size);

            struct passwd *pw = numeric_ids ? NULL : getpwuid(ent->st.st_uid);
            char owner_buf[32];
            if (pw)
                snprintf(owner_buf, sizeof(owner_buf), "%s", pw->pw_name);
            else
                snprintf(owner_buf, sizeof(owner_buf), "%u", ent->st.st_uid);

            struct group *gr = numeric_ids ? NULL : getgrgid(ent->st.st_gid);
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
            const time_t *tptr = &ent->st.st_mtime;
            if (time_word) {
                if (strcmp(time_word, "access") == 0 || strcmp(time_word, "use") == 0)
                    tptr = &ent->st.st_atime;
                else if (strcmp(time_word, "status") == 0)
                    tptr = &ent->st.st_ctime;
            } else {
                if (sort_atime)
                    tptr = &ent->st.st_atime;
                else if (sort_ctime)
                    tptr = &ent->st.st_ctime;
            }
            struct tm *tm = localtime(tptr);
            strftime(time_buf, sizeof(time_buf), time_style, tm);

            if (show_blocks)
                printf("%*lu ", (int)block_w, blk);
            if (show_inode)
                printf("%10llu ", (unsigned long long)ent->st.st_ino);
            printf("%s %*lu ", perms, (int)link_w, (unsigned long)ent->st.st_nlink);
            if (!hide_owner)
                printf("%-*s ", (int)owner_w, owner_buf);
            if (!hide_group)
                printf("%-*s ", (int)group_w, group_buf);
            printf("%*s %s", (int)size_w, size_buf, time_buf);
            if (show_context) {
#if HAVE_SELINUX
                char *ctx = NULL;
                char fullpath[PATH_MAX];
                snprintf(fullpath, sizeof(fullpath), "%s/%s", path, ent->name);
                if (lgetfilecon(fullpath, &ctx) >= 0) {
                    printf(" %s", ctx);
                    freecon(ctx);
                } else {
                    printf(" -");
                }
#else
                printf(" -");
#endif
            }
            printf(" %s", prefix);
            print_quoted(ent->name, quoting_style, hide_control);
            printf("%s%s\n", suffix, indicator);
        } else {
            if (show_blocks)
                printf("%*lu ", (int)block_w, blk);
            if (show_inode) {
                printf("%10llu %s", (unsigned long long)ent->st.st_ino, prefix);
                print_quoted(ent->name, quoting_style, hide_control);
                printf("%s%s\n", suffix, indicator);
            } else {
                fputs(prefix, stdout);
                print_quoted(ent->name, quoting_style, hide_control);
                printf("%s%s\n", suffix, indicator);
            }
        }
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
            list_directory(fullpath, color_mode, show_hidden, almost_all, long_format, show_inode, sort_time, sort_atime, sort_ctime, sort_size, sort_extension, sort_version, sort_word, unsorted, reverse, dirs_first, recursive, indicator_style, human_readable, numeric_ids, hide_owner, hide_group, show_context, follow_links, list_dirs_only, ignore_backups, ignore_patterns, ignore_count, hide_patterns, hide_count, columns, across_columns, one_per_line, comma_separated, show_blocks, quoting_style, time_word, time_style, block_size, hide_control);
        }
    }

cleanup:
    for (size_t i = 0; i < count; i++)
        free(entries[i].name);
    free(entries);
    closedir(dir);
}
