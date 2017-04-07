#!/bin/sh

#test script to test conditions of virtmem
#tests each program, and algorithm for 100 pages and 3-100 frames
#output is appended to results.txt file
#command ./virtmem $pages(100) $frames $sort $prog

if [ -f "results.txt" ]; then
    rm results.txt
fi

cmd='./virtmem 100 '
prg='scan sort focus'
alg='rand fifo custom'
fr='7 54 28'

for p in $prg ; do
    echo "program: $p" >> results.txt
    for a in $alg; do
        echo "algorithm: $a" >> results.txt
        for f in $fr ; do
            echo "frames: $f" >> results.txt
            echo "./virtmem 100 $f $a $p"
            ./virtmem 100 $f $a $p 2>&1 >/dev/null >> results.txt
        done
    done
done
