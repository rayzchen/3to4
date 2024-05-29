# 3to4

A virtual simulator for the [physical 3x3x3x3 design](https://hypercubing.xyz/puzzles/physical/3x3x3x3/) by Grant S.
Written with [GLFW](https://www.glfw.org/), [OpenGL](https://www.opengl.org/) and [C++](https://isocpp.org/).
Credit to [Akkei's physical 3^4 program](https://hypercubing.xyz/software/#other) for inspiration and design.

## Compiling

On Linux, install a C++ compiler, `make`, the required GLFW and OpenGL dev packages.

Then run
```
$ make
$ ./3to4
```
to compile and run.

On Windows, install [MinGW](https://www.mingw-w64.org/) or [Cygwin](https://www.cygwin.com/index.html) for a GNU C++ compiler. Next, go to https://www.glfw.org/download.html, select Windows binaries for the correct architecture. In the zip, copy all files under `lib-mingw-w64/` into this repo's `lib/` folder, and copy `include/GLFW/` into this repo's `include/` fonder.

Finally run
```
> make
> 3to4.exe
```
