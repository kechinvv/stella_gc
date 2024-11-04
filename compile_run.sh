#!/bin/bash

mkdir -p ./out

gcc -std=c11 -DSTELLA_DEBUG -DSTELLA_GC_STATS -DSTELLA_RUNTIME_STATS -DGC_LOGS $1 stella/runtime.c stella/gc.c stella/runtime.h stella/gc.h -o ./out/runnable_example

chmod a+x ./out/runnable_example

export GC_SPACE_SIZE=${3:-"1300"}

echo $2 | ./out/runnable_example
