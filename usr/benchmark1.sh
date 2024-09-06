#!/bin/bash

start_time=$(date +%s.%6N)

max1=10000
max2=1000
cons=2

z=0
i=0
while [ $i -le $max1 ]
do
  j=0
  while [ $j -le $max2 ]
  do
    z=$(( $cons * $z + $j ))
    j=$(( $j + 1 ))
  done
  i=$(( $i + 1 ))
done

end_time=$(date +%s.%6N)
echo "$start_time $end_time" | awk '{printf "BA Benchmark: %.5fs\n", $2 - $1}'
