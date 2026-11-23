# openpref

A card game

[![openpref.png](https://i.postimg.cc/Y2m09Ft2/openpref.png)](https://postimg.cc/0rPPXb6h)

OpenPref is an open source implementation of European trick-taking game Preferans (or Preference) against two virtual players. Information about game's rules and strategy can be easily found in Wikipedia: http://en.wikipedia.org/wiki/Preferans.

Originally forked from https://sourceforge.net/projects/openpref/ and converted to QT5

## Building

OpenPref needs Qt 5.15 (Widgets only) and CMake 3.16 or newer.

```sh
mkdir build && cd build && cmake .. && make
```

The binary is `build/openpref`. Run it from there; all images are compiled into
the executable, so it does not need the `src/pics` directory at run time.

```sh
make clean   # remove the build products
```

`make install` is not required.

On Haiku, install `qt5`, `qt5_devel` and `qt5_tools`. If CMake does not find Qt
by itself, point it at the system development tree:

```sh
cmake -DCMAKE_PREFIX_PATH=/boot/system/develop ..
```
