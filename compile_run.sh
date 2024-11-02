#!/bin/bash

mkdir -p ./out

gcc -std=c11 -DSTELLA_DEBUG -DSTELLA_GC_STATS -DSTELLA_RUNTIME_STATS $1 src/runtime.c src/gc.c src/runtime.h src/gc.h -o ./out/runnable_example

chmod a+x ./out/runnable_example

echo $2 | ./out/runnable_example