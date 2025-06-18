# vls
ls replacement utility for UNIX

vls is a minimal tool intended as a replacement for the standard `ls` command.

File names are colorized based on type (directories, links and executables).
Pass `--no-color` to disable colored output.
Use `-r` to display entries in reverse alphabetical order.
Use `-t` to sort entries by modification time.
Use `-S` to sort entries by file size.
Use `-i` to display inode numbers.
Use `-R` to recursively list subdirectories (symbolic links are not followed).
Use `-F` to append indicators to entries: `/` for directories, `*` for executables and `@` for symbolic links.
Use `-h` to display file sizes in human readable units when combined with `-l`.
Use `-L` to follow symbolic links when retrieving file details (the default is to display information about the links themselves).

Example of long format output:

```sh
$ ./build/vls -l
vls 0.1
-rw-r--r-- 1 user group 35149 Jun 18 16:55 LICENSE
-rw-r--r-- 1 user group  1104 Jun 18 16:55 Makefile
-rw-r--r-- 1 user group  1512 Jun 18 16:55 README.md
```

## Building and Testing
The build system uses a simple Makefile which detects the host OS with
`uname` and appends a few platform specific flags. In most cases you only
need to run `make`:

```sh
make
```

### Linux
On Linux the Makefile defines `-D_GNU_SOURCE` automatically. Simply run the
commands above. To run the basic tests:

```sh
make test
```

### macOS
The Makefile adds `-D_DARWIN_C_SOURCE` on macOS. Use the same build and test
commands:

```sh
make
make test
```

### NetBSD
On NetBSD the flag `-D_NETBSD_SOURCE` is applied. Invoke make normally:

```sh
make
make test
```

You can also provide your own `CFLAGS` to override or extend the defaults:

```sh
make CFLAGS="-O2"
```

The resulting executable is placed in `build/vls`.

