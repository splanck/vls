# vls

A minimal, colorized replacement for `ls`.

## Features
- Colorizes output based on file type with `LS_COLORS` customization
- Supports long listings and sorting with `--sort=WORD` (time, size, atime, ctime, extension, version, none)
- Recursive listing and directory-first ordering
- Pattern ignoring and indicator characters ("/", "*", "@"); use `--file-type` for directory markers only
- Hide entries matching glob patterns with `--hide=PATTERN`
- Optional dereferencing of command line symlinks (`-H`)
- Optional display of SELinux contexts (`-Z`, Linux only)
- Options for quoting names, human readable sizes, column layout (`-C`/`-x`) and comma-separated output (`-m`)
- Backslash-escapes for non-printable characters (`-b`)
- Customizable timestamp format via `--time-style=FMT` and `--full-time`
- Select which timestamp to show with `--time=WORD` (`mod`, `access`, `use`, `status`)
- Cross‑platform Makefile for Linux, macOS and NetBSD

For a complete list of options see [vlsdoc.md](./vlsdoc.md) or the manual page at [man/vls.1](./man/vls.1).

## Building
Run `make` to compile. Override `CFLAGS` or `PREFIX` as needed.

## Installation
Install with `sudo make install`. Use `PREFIX` to choose a different destination.

## License
Distributed under the GNU General Public License version 3. See [LICENSE](./LICENSE) for details.
