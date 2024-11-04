#!/bin/bash

mkdir -p ./out

gcc -std=c11 -DSTELLA_DEBUG -DSTELLA_GC_STATS -DSTELLA_RUNTIME_STATS $1 stella/runtime.c stella/gc.c stella/runtime.h stella/gc.h -o ./out/runnable_example

chmod a+x ./out/runnable_example

echo $2 | ./out/runnable_example