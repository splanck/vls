# vls Manual

## Name
vls - colorized ls replacement

## Synopsis
`vls [OPTION]... [FILE]...`

## Description
`vls` is a minimal utility that lists directory contents similarly to `ls`. File names are colorized by default. The option `--color=WHEN` controls coloring where WHEN is `auto`, `always` or `never`. The default is to display information about symbolic links. Use `-L` to follow them.

## Options
- `-a` Include directory entries whose names begin with a dot (.).
- `-A`, `--almost-all` List all entries except `.` and `..`.
- `-l` Use a long listing format.
- `-i` Print the inode number of each file.
- `-t` Sort by modification time, newest first.
- `-u` Sort by access time, newest first. When combined with `-l`, display access time instead of modification time.
- `-c` Sort by status change time, newest first. When combined with `-l`, display change time instead of modification time.
- `-S` Sort by file size, largest first.
- `-X` Sort by file extension, case-insensitive.
- `-f`, `-U` Do not sort; list entries in directory order.
- `--group-directories-first` List directories before other files.
- `-r` Reverse the sort order.
- `-R` List subdirectories recursively (symbolic links are not followed).
- `-d` List directory arguments themselves instead of their contents.
- `-L` Follow symbolic links when retrieving file details.
- `-F` Append indicator characters to entries: `/` for directories, `*` for executables and `@` for symbolic links.
- `-p` Append '/' to directory names.
- `-s` Display the number of blocks allocated to each file. When used with `-l` or `-s`, a line of the form `total <num>` appears before the listing showing the sum of blocks for the displayed files according to the current block size.
- `--block-size=SIZE` Override the default block size (512 or 1024 bytes).
- `-k` Use 1 KiB blocks for size calculations.
- `-h` With `-l`, print sizes in human readable format.
- `-n` Display numeric user and group IDs in long format output.
- `-g` Omit the owner column in long format output.
- `-o` Omit the group column in long format output.
- `-B`, `--ignore-backups` Do not list files ending with '~'.
- `-I PATTERN`, `--ignore=PATTERN` Do not list entries matching the shell PATTERN. May be repeated.
- `-C` List entries vertically in columns (default for terminals).
- `-x` List entries across columns instead of vertically.
- `-m` List entries separated by ", " wrapping lines to terminal width.
- `-Q`, `--quote-name` Quote file names with double quotes, escaping internal quotes and backslashes.
- `-1` List one entry per line.
- `--color=WHEN` Control colorization. WHEN is `auto`, `always` or `never`.
- `--help` Display a brief usage message and exit.
- `-V`, `--version` Display the program version and exit.

## Environment
- `LS_COLORS` - When set, overrides the default color codes. Use keys `di` for directories, `ln` for symbolic links, `ex` for executables and `rs` for the reset sequence.

## Examples
```sh
vls
vls -al
vls -tR /etc
vls --color=never
vls -I '*.o'
```

## See Also
`ls(1)`
