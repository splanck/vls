# vls
ls replacement utility for UNIX

vls is a minimal tool intended as a replacement for the standard `ls` command.

File names are colorized based on type (directories, links and executables).
Pass `--no-color` to disable colored output.
Use `-r` to display entries in reverse alphabetical order.
Use `-t` to sort entries by modification time.

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

