# vls Manual

## Name
vls - colorized ls replacement

## Synopsis
`vls [OPTION]... [FILE]...`

## Description
`vls` is a minimal utility that lists directory contents similarly to `ls`. File names are colorized by default. The option `--color=WHEN` controls coloring where WHEN is `auto`, `always` or `never`. The default is to display information about symbolic links. Use `-L` to follow them or `-H` for command line paths only.

## Options
- `-a` Include directory entries whose names begin with a dot (.).
- `-A`, `--almost-all` Show all dot files except `.` and `..`.
- `-l` Use a long listing format.
- `-i` Print the inode number of each file.
- `-t` Sort by modification time, newest first.
- `-u` Sort by access time, newest first. When combined with `-l`, display access time instead of modification time.
- `-c` Sort by status change time, newest first. When combined with `-l`, display change time instead of modification time.
- `-S` Sort by file size, largest first.
- `-X` Sort by file extension, case-insensitive.
- `-v` Sort by version (natural order).
- `--sort=WORD` Choose sort field: `size`, `time`, `atime`, `ctime`,
  `extension`, `version` or `none`.
- `-f`, `-U` Do not sort; list entries in directory order.
- `--group-directories-first` List directories before other files.
- `--time-style=FMT` Format times using `strftime(3)` style FMT. The output
  buffer is sized relative to `FMT`, so unusually long expansions may be
  truncated.
- `--full-time` Equivalent to `--time-style="%F %T %z"`.
- `--time=WORD` Choose which timestamp field to display: `mod` (default),
  `access`, `use` or `status`.
- `-r` Reverse the sort order.
- `-R` List subdirectories recursively (symbolic links are not followed).
- `-d` List directory arguments themselves instead of their contents.
- `-L` Follow symbolic links when retrieving file details.
- `-H` Follow symbolic links specified on the command line.
- `-Z` Print SELinux context before the file name (Linux only).
- `-F` Append indicator characters to entries: `/` for directories, `*` for executables and `@` for symbolic links.
- `-p` Append '/' to directory names.
- `--file-type` Like `-p` but ignores other indicators unless `-F` is also given.
- `--indicator-style=STYLE` Choose indicator style: `none`, `slash`, `file-type`, `classify`.
- `-s` Display the number of blocks allocated to each file. When used with `-l` or `-s`, a line of the form `total <num>` appears before the listing showing the sum of blocks for the displayed files according to the current block size.
- `--block-size=SIZE` Override the default block size. `SIZE` is a number of
  bytes with no unit suffix.
- `-k` Use 1 KiB blocks for size calculations.
- `-h` With `-l`, print sizes in human readable format using powers of 1024.
- `--si` Like `-h` but use powers of 1000.
- `-n` Display numeric user and group IDs in long format output.
- `-g` Omit the owner column in long format output.
- `-o` Omit the group column in long format output.
- `-B`, `--ignore-backups` Do not list files ending with '~'.
- `-I PATTERN`, `--ignore=PATTERN` Do not list entries matching the shell PATTERN. May be repeated.
- `--hide=PATTERN` Hide entries matching PATTERN unless `-a` or `-A` is used. May be repeated.
- `-C` List entries vertically in columns (default for terminals).
- `-x` List entries across columns instead of vertically.
- `-m` List entries separated by ", " wrapping lines to terminal width.
- `-w COLS`, `--width=COLS` Set the output width.
- `-T COLS`, `--tabsize=COLS` Set tab width for column calculations.
- `-Q`, `--quote-name` Use C-style quoting for file names.
- `-b` Use backslash escapes for non-printable characters.
- `--quoting-style=STYLE` Select quoting style: `literal`, `c`, `escape`.
- `-N`, `--literal` Print file names literally, overriding quoting and escaping options.
- `-q`, `--hide-control-chars` Show `?` instead of non-printable characters.
- `--show-control-chars` Display control characters directly.
- `-1` List one entry per line.
- `--color=WHEN` Control colorization. WHEN is `auto`, `always` or `never`.
- `--hyperlink=WHEN` Wrap file names in OSC 8 hyperlinks when WHEN is `auto`,
  `always` or `never`.
- `--help` Display a brief usage message and exit.
- `-V`, `--version` Display the program version and exit.

## Environment
- `LS_COLORS` - When set, overrides the default color codes. Use keys `di` for directories, `ln` for symbolic links, `ex` for executables and `rs` for the reset sequence.
- SELinux context display (`-Z`) is only available on Linux systems with the SELinux library installed.

## Examples
```sh
vls
vls -al
vls -tR /etc
vls --color=never
vls -I '*.o'
vls --hide='*.tmp'
vls -b
vls --indicator-style=classify
vls --hyperlink=always
```

## See Also
`ls(1)`

## Notes
Earlier versions always displayed `1` as the link count when listing a
single directory with `-l`. The count now shows the actual number of
hard links.
