#!/bin/sh

make local

rm nodes.txt || true

for i in `seq 0 $(( $1 - 1 ))`
do
    echo "$i 127.0.0.1 $(( 8888 + $i ))" >> nodes.txt
done

for i in `seq 0 $(( $1 - 1 ))`
do
    xterm  -hold -e ./workload $i &
done
