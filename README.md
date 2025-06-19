# vls

A minimal, colorized replacement for `ls`.

## Features
- Colorizes output based on file type with `LS_COLORS` customization
- Supports long listings and various sorting modes (time, size, extension)
- Recursive listing and directory-first ordering
- Pattern ignoring and indicator characters ("/", "*", "@")
- Options for quoting names, human readable sizes and column layout (`-C`/`-x`)
- Cross‑platform Makefile for Linux, macOS and NetBSD

For a complete list of options see [vlsdoc.md](./vlsdoc.md) or the manual page at [man/vls.1](./man/vls.1).

## Building
Run `make` to compile. Override `CFLAGS` or `PREFIX` as needed.

## Installation
Install with `sudo make install`. Use `PREFIX` to choose a different destination.

## License
Distributed under the GNU General Public License version 3. See [LICENSE](./LICENSE) for details.
