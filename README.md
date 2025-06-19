# vls

A minimal, colorized replacement for `ls`.

## Features
- Colorizes output based on file type with `LS_COLORS` customization
- Optional OSC 8 hyperlinks with `--hyperlink=WHEN`
- Supports long listings and sorting with `--sort=WORD`
  (time, size, atime, ctime, extension, version, none)
- Recursive listing and directory-first ordering
- Indicator characters configurable with `--indicator-style=STYLE`
  (`none`, `slash`, `file-type`, `classify`)
- Pattern ignoring and indicator characters ("/", "*", "@"); use
  `--file-type` for directory markers only
- Hide entries matching glob patterns with `--hide=PATTERN`
- Optional dereferencing of command line symlinks (`-H`)
- Optional display of SELinux contexts (`-Z`, Linux only, requires libselinux)
- Quoting styles with `--quoting-style=STYLE`
  (`literal`, `c`, `escape`), with `-Q` and `-b` as shortcuts
- Print names literally with `-N`/`--literal`, overriding quoting
  and escaping options
- Hide control characters with `-q`/`--hide-control-chars`
- Display control characters literally with `--show-control-chars`
- Human readable sizes (`-h` uses powers of 1024, `--si` uses powers of
  1000), column layout (`-C`/`-x`) and comma-separated output (`-m`)
- Override the block size used for `-s` with `--block-size=SIZE` where `SIZE`
  is a number of bytes with no unit suffix; use `-k` for 1 KiB blocks
- Set output width with `-w COLS` and tab size with `-T COLS`
- Customizable timestamp format via `--time-style=FMT` and `--full-time`
- Select which timestamp to show with `--time=WORD` (`mod`, `access`,
  `use`, `status`)
- Cross‑platform Makefile for Linux, macOS and NetBSD

For a complete list of options see [vlsdoc.md](./vlsdoc.md) or the manual page
at [man/vls.1](./man/vls.1).

## Building
Run `make` to compile. Override `CFLAGS` or `PREFIX` as needed.
SELinux support requires the libselinux development package to be installed.

## Installation
Install with `sudo make install`. Use `PREFIX` to choose a different
destination.

## Testing
Run `make test` to compile `vls` and execute a small regression suite.
The tests create a temporary directory under `build/` and exercise
options such as `-A`, `-d`, `-C`, `-m` and both `--color=always` and
`--color=never`. Output and the exit status for each invocation are
saved in `build/` for inspection. The script verifies that every command
succeeds and prints expected data. A message is printed once all tests
pass.

## License
Distributed under the GNU General Public License version 3.
See [LICENSE](./LICENSE) for details.
