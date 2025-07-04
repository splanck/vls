#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <wchar.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <fnmatch.h>
#include <stdbool.h>
#ifndef HAVE_SELINUX
# define HAVE_SELINUX 0
#endif

#if HAVE_SELINUX
# ifdef __has_include
#  if __has_include(<selinux/selinux.h>)
#   include <selinux/selinux.h>
#  else
#   undef HAVE_SELINUX
#   define HAVE_SELINUX 0
#  endif
# else
#  include <selinux/selinux.h>
# endif
#endif
#include "list.h"
#include "color.h"
#include "util.h"
#include "quote.h"

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

typedef struct {
    char *name;
    struct stat st;
} Entry;

typedef struct Visited {
    dev_t dev;
    ino_t ino;
    struct Visited *next;
} Visited;

static Visited *visited_head = NULL;
static int recursion_depth = 0;

static int visited_contains(dev_t dev, ino_t ino) {
    for (Visited *v = visited_head; v; v = v->next)
        if (v->dev == dev && v->ino == ino)
            return 1;
    return 0;
}

static int visited_add(dev_t dev, ino_t ino) {
    Visited *v = malloc(sizeof(Visited));
    if (!v)
        return -1;
    v->dev = dev;
    v->ino = ino;
    v->next = visited_head;
    visited_head = v;
    return 0;
}

static void visited_free_all(void) {
    Visited *v = visited_head;
    while (v) {
        Visited *tmp = v->next;
        free(v);
        v = tmp;
    }
    visited_head = NULL;
}

#define FINALIZE() do {                         \
    recursion_depth--;                           \
    if (recursion_depth == 0)                    \
        visited_free_all();                      \
} while (0)

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

static void human_size(off_t size, int si, char *buf, size_t bufsz) {
    const char suffixes[] = {'B','K','M','G','T','P'};
    double s = (double)size;
    int i = 0;
    int base = si ? 1000 : 1024;
    while (s >= base && i < 5) {
        s /= base;
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
    mbstate_t st;
    memset(&st, 0, sizeof(st));
    const char *p = s;
    while (*p) {
        wchar_t wc;
        size_t n = mbrtowc(&wc, p, MB_CUR_MAX, &st);
        if (n == (size_t)-1 || n == (size_t)-2) {
            wc = (unsigned char)*p;
            n = 1;
            memset(&st, 0, sizeof(st));
        }
        if (wc == L'"' || wc == L'\\')
            len++; /* for escape */
        int w = wcwidth(wc);
        if (w < 0) {
            if (hide_control)
                len += 1;
            else if (escape_nonprint)
                len += 4; /* backslash + 3 octal digits */
            else
                len += 1;
        } else {
            len += w;
        }
        p += n;
    }
    return len;
}

static size_t escaped_len(const char *s, int hide_control) {
    size_t len = 0;
    mbstate_t st;
    memset(&st, 0, sizeof(st));
    const char *p = s;
    while (*p) {
        wchar_t wc;
        size_t n = mbrtowc(&wc, p, MB_CUR_MAX, &st);
        if (n == (size_t)-1 || n == (size_t)-2) {
            wc = (unsigned char)*p;
            n = 1;
            memset(&st, 0, sizeof(st));
        }
        int w = wcwidth(wc);
        if (w < 0) {
            if (hide_control)
                len += 1;
            else
                len += 4; /* backslash + 3 octal digits */
        } else {
            len += w;
        }
        p += n;
    }
    return len;
}


void list_directory(const char *path, ColorMode color_mode, HyperlinkMode hyperlink_mode, int show_hidden, int almost_all, int long_format, int show_inode, int sort_time, int sort_atime, int sort_ctime, int sort_size, int sort_extension, int sort_version, const char *sort_word, int unsorted, int reverse, int dirs_first, int recursive, IndicatorStyle indicator_style, int human_readable, int human_si, int numeric_ids, int hide_owner, int hide_group, int show_context, int follow_links, int list_dirs_only, int ignore_backups, const char **ignore_patterns, size_t ignore_count, const char **hide_patterns, size_t hide_count, int columns, int across_columns, int one_per_line, int comma_separated, int output_width, int tabsize, int show_blocks, QuotingStyle quoting_style, const char *time_word, const char *time_style, unsigned block_size, int hide_control, int show_controls, int literal_names) {
    recursion_depth++;
    if (follow_links) {
        struct stat vst;
        if (stat(path, &vst) == 0) {
            if (visited_contains(vst.st_dev, vst.st_ino)) {
                fprintf(stderr, "warning: skipping cyclic directory '%s'\n", path);
                FINALIZE();
                return;
            }
            if (visited_add(vst.st_dev, vst.st_ino) == -1) {
                perror("malloc");
                FINALIZE();
                return;
            }
        }
    }
    int use_color = 0;
    if (color_mode == COLOR_ALWAYS)
        use_color = 1;
    else if (color_mode == COLOR_AUTO)
        use_color = isatty(STDOUT_FILENO);
    if (literal_names) {
        quoting_style = QUOTE_LITERAL;
        hide_control = 0;
        show_controls = 0;
    }
    int quote_names = (quoting_style == QUOTE_C);
    int escape_nonprint = (quoting_style == QUOTE_C || quoting_style == QUOTE_ESCAPE);
    if (show_controls) {
        hide_control = 0;
        escape_nonprint = 0;
    }

    long pw_bufsz_l = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (pw_bufsz_l < 0)
        pw_bufsz_l = 16384;
    size_t pw_bufsz = (size_t)pw_bufsz_l;
    char *pwbuf = malloc(pw_bufsz);
    if (!pwbuf) {
        perror("malloc");
        FINALIZE();
        return;
    }
    long gr_bufsz_l = sysconf(_SC_GETGR_R_SIZE_MAX);
    if (gr_bufsz_l < 0)
        gr_bufsz_l = 16384;
    size_t gr_bufsz = (size_t)gr_bufsz_l;
    char *grbuf = malloc(gr_bufsz);
    if (!grbuf) {
        perror("malloc");
        free(pwbuf);
        FINALIZE();
        return;
    }
    if (list_dirs_only) {
        struct stat st;
        int (*stat_fn)(const char *, struct stat *) = follow_links ? stat : lstat;
        if (stat_fn(path, &st) == -1) {
            fprintf(stderr, "stat: %s: %s\n", path, strerror(errno));
            free(pwbuf);
            free(grbuf);
            FINALIZE();
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
        size_t link_w = num_digits(st.st_nlink);

        if (long_format) {
            char size_buf[16];
            if (human_readable)
                human_size(st.st_size, human_si, size_buf, sizeof(size_buf));
            else
                snprintf(size_buf, sizeof(size_buf), "%lld", (long long)st.st_size);

            struct passwd pw;
            struct passwd *pw_res = NULL;
            const char *owner_buf = NULL;
            char owner_num[32];
            if (!numeric_ids && getpwuid_r(st.st_uid, &pw, pwbuf, pw_bufsz, &pw_res) == 0 && pw_res)
                owner_buf = pw_res->pw_name;
            else {
                snprintf(owner_num, sizeof(owner_num), "%u", st.st_uid);
                owner_buf = owner_num;
            }

            struct group gr;
            struct group *gr_res = NULL;
            const char *group_buf = NULL;
            char group_num[32];
            if (!numeric_ids && getgrgid_r(st.st_gid, &gr, grbuf, gr_bufsz, &gr_res) == 0 && gr_res)
                group_buf = gr_res->gr_name;
            else {
                snprintf(group_num, sizeof(group_num), "%u", st.st_gid);
                group_buf = group_num;
            }


            char perms[11];
            perms[0] = S_ISDIR(st.st_mode) ? 'd' :
                       S_ISLNK(st.st_mode) ? 'l' :
                       S_ISCHR(st.st_mode) ? 'c' :
                       S_ISBLK(st.st_mode) ? 'b' :
                       S_ISFIFO(st.st_mode) ? 'p' :
                       S_ISSOCK(st.st_mode) ? 's' : '-';
            perms[1] = (st.st_mode & S_IRUSR) ? 'r' : '-';
            perms[2] = (st.st_mode & S_IWUSR) ? 'w' : '-';
            perms[3] = (st.st_mode & S_IXUSR)
                        ? ((st.st_mode & S_ISUID) ? 's' : 'x')
                        : ((st.st_mode & S_ISUID) ? 'S' : '-');
            perms[4] = (st.st_mode & S_IRGRP) ? 'r' : '-';
            perms[5] = (st.st_mode & S_IWGRP) ? 'w' : '-';
            perms[6] = (st.st_mode & S_IXGRP)
                        ? ((st.st_mode & S_ISGID) ? 's' : 'x')
                        : ((st.st_mode & S_ISGID) ? 'S' : '-');
            perms[7] = (st.st_mode & S_IROTH) ? 'r' : '-';
            perms[8] = (st.st_mode & S_IWOTH) ? 'w' : '-';
            perms[9] = (st.st_mode & S_IXOTH)
                        ? ((st.st_mode & S_ISVTX) ? 't' : 'x')
                        : ((st.st_mode & S_ISVTX) ? 'T' : '-');
            perms[10] = '\0';

            size_t time_buf_sz = strlen(time_style) * 4 + 32;
            char *time_buf = malloc(time_buf_sz);
            if (!time_buf) {
                perror("malloc");
                goto cleanup;
            }
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
            strftime(time_buf, time_buf_sz, time_style, tm);

            size_t owner_len = strlen(owner_buf);
            size_t group_len = strlen(group_buf);

            if (show_blocks)
                printf("%*lu ", (int)single_w, single_blocks);
            if (show_inode)
                printf("%10llu ", (unsigned long long)st.st_ino);
            printf("%s %*lu ", perms, (int)link_w, (unsigned long)st.st_nlink);
            if (!hide_owner)
                printf("%-*s ", (int)owner_len, owner_buf);
            if (!hide_group)
                printf("%-*s ", (int)group_len, group_buf);
            printf("%*s %s", (int)strlen(size_buf), size_buf, time_buf);
            free(time_buf);
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
            hyperlink_start(path, hyperlink_mode);
            print_quoted(path, quoting_style, hide_control, show_controls, literal_names);
            hyperlink_end(hyperlink_mode);
            printf("%s%s\n", suffix, indicator);
        } else {
            if (show_blocks)
                printf("%*lu ", (int)single_w, single_blocks);
            if (show_inode) {
                printf("%10llu %s", (unsigned long long)st.st_ino, prefix);
                hyperlink_start(path, hyperlink_mode);
                print_quoted(path, quoting_style, hide_control, show_controls, literal_names);
                hyperlink_end(hyperlink_mode);
                printf("%s%s\n", suffix, indicator);
            }
            else {
                fputs(prefix, stdout);
                hyperlink_start(path, hyperlink_mode);
                print_quoted(path, quoting_style, hide_control, show_controls, literal_names);
                hyperlink_end(hyperlink_mode);
                printf("%s%s\n", suffix, indicator);
            }
        }
        FINALIZE();
        return;
    }

    DIR *dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "opendir: %s: %s\n", path, strerror(errno));
        free(pwbuf);
        free(grbuf);
        FINALIZE();
        return;
    }

    if (recursive) {
        hyperlink_start(path, hyperlink_mode);
        print_quoted(path, quoting_style, hide_control, show_controls, literal_names);
        hyperlink_end(hyperlink_mode);
        printf(":\n");
    }

    struct dirent *entry;
    size_t count = 0, capacity = 32;
    Entry *entries = malloc(capacity * sizeof(Entry));
    if (!entries) {
        perror("malloc");
        closedir(dir);
        free(pwbuf);
        free(grbuf);
        FINALIZE();
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (!show_hidden && !almost_all && entry->d_name[0] == '.')
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
        char *fullpath = join_path(path, entries[count].name);
        if (!fullpath) {
            perror("malloc");
            goto cleanup;
        }
        int (*stat_fn)(const char *, struct stat *) = follow_links ? stat : lstat;
        if (stat_fn(fullpath, &entries[count].st) == -1) {
            fprintf(stderr, "stat: %s: %s\n", fullpath, strerror(errno));
            free(fullpath);
            free(entries[count].name);
            continue;
        }
        free(fullpath);
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
                struct passwd pw;
                struct passwd *pw_res = NULL;
                size_t len;
                if (!numeric_ids && getpwuid_r(ent->st.st_uid, &pw, pwbuf, pw_bufsz, &pw_res) == 0 && pw_res)
                    len = strlen(pw_res->pw_name);
                else
                    len = num_digits(ent->st.st_uid);
                if (len > owner_w)
                    owner_w = len;
            }

            if (!hide_group) {
                struct group gr;
                struct group *gr_res = NULL;
                size_t len;
                if (!numeric_ids && getgrgid_r(ent->st.st_gid, &gr, grbuf, gr_bufsz, &gr_res) == 0 && gr_res)
                    len = strlen(gr_res->gr_name);
                else
                    len = num_digits(ent->st.st_gid);
                if (len > group_w)
                    group_w = len;
            }

            char sz[16];
            if (human_readable)
                human_size(ent->st.st_size, human_si, sz, sizeof(sz));
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
        if (use_color) {
            const char *pfx = "";
            if (S_ISDIR(ent->st.st_mode))
                pfx = color_dir();
            else if (S_ISLNK(ent->st.st_mode))
                pfx = color_link();
            else if (ent->st.st_mode & S_IXUSR)
                pfx = color_exec();
            name_len += strlen(pfx) + strlen(color_reset());
        }
        if (name_len > max_len)
            max_len = name_len;
    }

    if (show_blocks)
        max_len += block_w + 1;

    if (long_format || show_blocks)
        printf("total %lu\n", total_blocks);

    if (comma_separated && !long_format) {
        int term_width = output_width;
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
                         strlen(indicator) + strlen(prefix) + strlen(suffix);
            if (line_len && line_len + len > (size_t)term_width) {
                putchar('\n');
                line_len = 0;
            }
            char *fullpath = join_path(path, ent->name);
            if (!fullpath) {
                perror("malloc");
                goto cleanup;
            }
            printf("%s%s%s", block_buf, inode_buf, prefix);
            hyperlink_start(fullpath, hyperlink_mode);
            print_quoted(ent->name, quoting_style, hide_control, show_controls, literal_names);
            hyperlink_end(hyperlink_mode);
            printf("%s%s", suffix, indicator);
            free(fullpath);
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
        if (count == 0) {
            putchar('\n');
        } else {
            int term_width = output_width;
            size_t col_width = ((max_len + tabsize - 1) / tabsize) * tabsize + 2;
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
            char *fullpath = join_path(path, ent->name);
            if (!fullpath) {
                perror("malloc");
                goto cleanup;
            }
            printf("%s%s%s", block_buf, inode_buf, prefix);
            hyperlink_start(fullpath, hyperlink_mode);
            print_quoted(ent->name, quoting_style, hide_control, show_controls, literal_names);
            hyperlink_end(hyperlink_mode);
            printf("%s%s", suffix, indicator);
            free(fullpath);

            size_t len = (quote_names ? quoted_len(ent->name, escape_nonprint, hide_control) :
                           (escape_nonprint ? escaped_len(ent->name, hide_control) : strlen(ent->name))) +
                         strlen(indicator) + strlen(inode_buf) + strlen(block_buf) +
                         strlen(prefix) + strlen(suffix);
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
                    char *fullpath = join_path(path, ent->name);
                    if (!fullpath) {
                        perror("malloc");
                        goto cleanup;
                    }
                    printf("%s%s%s", block_buf, inode_buf, prefix);
                    hyperlink_start(fullpath, hyperlink_mode);
                    print_quoted(ent->name, quoting_style, hide_control, show_controls, literal_names);
                    hyperlink_end(hyperlink_mode);
                    printf("%s%s", suffix, indicator);
                    free(fullpath);

                    size_t len = (quote_names ? quoted_len(ent->name, escape_nonprint, hide_control) :
                                   (escape_nonprint ? escaped_len(ent->name, hide_control) : strlen(ent->name))) +
                                 strlen(indicator) + strlen(inode_buf) + strlen(block_buf) +
                                 strlen(prefix) + strlen(suffix);
                    if (c == cols - 1 || i + rows >= count) {
                        putchar('\n');
                    } else {
                        for (size_t sp = len; sp < col_width; sp++)
                            putchar(' ');
                    }
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
                human_size(ent->st.st_size, human_si, size_buf, sizeof(size_buf));
            else
                snprintf(size_buf, sizeof(size_buf), "%lld", (long long)ent->st.st_size);

            struct passwd pw;
            struct passwd *pw_res = NULL;
            const char *owner_buf = NULL;
            char owner_num[32];
            if (!numeric_ids && getpwuid_r(ent->st.st_uid, &pw, pwbuf, pw_bufsz, &pw_res) == 0 && pw_res)
                owner_buf = pw_res->pw_name;
            else {
                snprintf(owner_num, sizeof(owner_num), "%u", ent->st.st_uid);
                owner_buf = owner_num;
            }

            struct group gr;
            struct group *gr_res = NULL;
            const char *group_buf = NULL;
            char group_num[32];
            if (!numeric_ids && getgrgid_r(ent->st.st_gid, &gr, grbuf, gr_bufsz, &gr_res) == 0 && gr_res)
                group_buf = gr_res->gr_name;
            else {
                snprintf(group_num, sizeof(group_num), "%u", ent->st.st_gid);
                group_buf = group_num;
            }

            char perms[11];
            perms[0] = S_ISDIR(ent->st.st_mode) ? 'd' :
                       S_ISLNK(ent->st.st_mode) ? 'l' :
                       S_ISCHR(ent->st.st_mode) ? 'c' :
                       S_ISBLK(ent->st.st_mode) ? 'b' :
                       S_ISFIFO(ent->st.st_mode) ? 'p' :
                       S_ISSOCK(ent->st.st_mode) ? 's' : '-';
            perms[1] = (ent->st.st_mode & S_IRUSR) ? 'r' : '-';
            perms[2] = (ent->st.st_mode & S_IWUSR) ? 'w' : '-';
            perms[3] = (ent->st.st_mode & S_IXUSR)
                        ? ((ent->st.st_mode & S_ISUID) ? 's' : 'x')
                        : ((ent->st.st_mode & S_ISUID) ? 'S' : '-');
            perms[4] = (ent->st.st_mode & S_IRGRP) ? 'r' : '-';
            perms[5] = (ent->st.st_mode & S_IWGRP) ? 'w' : '-';
            perms[6] = (ent->st.st_mode & S_IXGRP)
                        ? ((ent->st.st_mode & S_ISGID) ? 's' : 'x')
                        : ((ent->st.st_mode & S_ISGID) ? 'S' : '-');
            perms[7] = (ent->st.st_mode & S_IROTH) ? 'r' : '-';
            perms[8] = (ent->st.st_mode & S_IWOTH) ? 'w' : '-';
            perms[9] = (ent->st.st_mode & S_IXOTH)
                        ? ((ent->st.st_mode & S_ISVTX) ? 't' : 'x')
                        : ((ent->st.st_mode & S_ISVTX) ? 'T' : '-');
            perms[10] = '\0';

            size_t time_buf_sz = strlen(time_style) * 4 + 32;
            char *time_buf = malloc(time_buf_sz);
            if (!time_buf) {
                perror("malloc");
                goto cleanup;
            }
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
            strftime(time_buf, time_buf_sz, time_style, tm);

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
            free(time_buf);
            if (show_context) {
#if HAVE_SELINUX
                char *ctx = NULL;
                char *fullpath = join_path(path, ent->name);
                if (!fullpath) {
                    perror("malloc");
                    goto cleanup;
                }
                if (lgetfilecon(fullpath, &ctx) >= 0) {
                    printf(" %s", ctx);
                    freecon(ctx);
                } else {
                    printf(" -");
                }
                free(fullpath);
#else
                printf(" -");
#endif
            }
            printf(" %s", prefix);
            char *fullpath1 = join_path(path, ent->name);
            if (!fullpath1) {
                perror("malloc");
                goto cleanup;
            }
            hyperlink_start(fullpath1, hyperlink_mode);
            print_quoted(ent->name, quoting_style, hide_control, show_controls, literal_names);
            hyperlink_end(hyperlink_mode);
            printf("%s%s\n", suffix, indicator);
            free(fullpath1);
        } else {
            if (show_blocks)
                printf("%*lu ", (int)block_w, blk);
            if (show_inode) {
                printf("%10llu %s", (unsigned long long)ent->st.st_ino, prefix);
                char *fullpath2 = join_path(path, ent->name);
                if (!fullpath2) {
                    perror("malloc");
                    goto cleanup;
                }
                hyperlink_start(fullpath2, hyperlink_mode);
                print_quoted(ent->name, quoting_style, hide_control, show_controls, literal_names);
                hyperlink_end(hyperlink_mode);
                printf("%s%s\n", suffix, indicator);
                free(fullpath2);
            } else {
                fputs(prefix, stdout);
                char *fullpath3 = join_path(path, ent->name);
                if (!fullpath3) {
                    perror("malloc");
                    goto cleanup;
                }
                hyperlink_start(fullpath3, hyperlink_mode);
                print_quoted(ent->name, quoting_style, hide_control, show_controls, literal_names);
                hyperlink_end(hyperlink_mode);
                printf("%s%s\n", suffix, indicator);
                free(fullpath3);
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
            char *fullpath = join_path(path, ent->name);
            if (!fullpath) {
                perror("malloc");
                goto cleanup;
            }
            if (follow_links) {
                struct stat vst;
                if (stat(fullpath, &vst) == 0 && visited_contains(vst.st_dev, vst.st_ino)) {
                    fprintf(stderr, "warning: skipping cyclic directory '%s'\n", fullpath);
                    free(fullpath);
                    continue;
                }
            }
            printf("\n");
            list_directory(fullpath, color_mode, hyperlink_mode, show_hidden, almost_all, long_format, show_inode, sort_time, sort_atime, sort_ctime, sort_size, sort_extension, sort_version, sort_word, unsorted, reverse, dirs_first, recursive, indicator_style, human_readable, human_si, numeric_ids, hide_owner, hide_group, show_context, follow_links, list_dirs_only, ignore_backups, ignore_patterns, ignore_count, hide_patterns, hide_count, columns, across_columns, one_per_line, comma_separated, output_width, tabsize, show_blocks, quoting_style, time_word, time_style, block_size, hide_control, show_controls, literal_names);
            free(fullpath);
        }
    }

cleanup:
    for (size_t i = 0; i < count; i++)
        free(entries[i].name);
    free(entries);
    closedir(dir);
    free(pwbuf);
    free(grbuf);
    FINALIZE();
}
