# Garbage Collector lab

[![CMake on a single platform](https://github.com/kechinvv/cc_gc/actions/workflows/cmake-single-platform.yml/badge.svg)](https://github.com/kechinvv/cc_gc/actions/workflows/cmake-single-platform.yml)

## Build

Run `compile_run.sh` with target `.c` example and program input. 3rd arg is size for GS_SPACE_SIZE (optional, default 1300).
```
sudo ./compile_run.sh ./stella_examples/factorial.c 4 1 1400
```
or build manually
```
gcc -std=c11 -DSTELLA_DEBUG -DSTELLA_GC_STATS -DSTELLA_RUNTIME_STATS example/path.c stella/runtime.c stella/gc.c stella/runtime.h stella/gc.h -o ./out/runnable_example
```
## Additional flags
`-DGC_LOGS` - enable gc logs
`-DSIMPLE_COPY` - simple copy gc mode
## ENV

`GC_SPACE_SIZE` - int (auto set in `compile_run.sh`).=